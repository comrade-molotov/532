# ============================================================================
#  SA-MP Freeroam 0.3.7 - plugin-ების ჩამოტვირთვა (Windows / PowerShell)
#
#  პირდაპირ გაშვება არ არის საჭირო - გამოიძახე download_dependencies.bat
#  ან setup.bat. სკრიპტი უსაფრთხოდ მეორდება (არსებულ ფაილებს არ შლის).
# ============================================================================
$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$Root    = Split-Path -Parent $MyInvocation.MyCommand.Path
$Plugins = Join-Path $Root 'plugins'
$Tmp     = Join-Path $env:TEMP ('samp-deps-' + [guid]::NewGuid().ToString('N'))

New-Item -ItemType Directory -Force -Path $Plugins, $Tmp | Out-Null

$Urls = @{
    mysql    = 'https://github.com/pBlueG/SA-MP-MySQL/releases/download/R41-4/mysql-R41-4-win32.zip'
    streamer = 'https://github.com/samp-incognito/samp-streamer-plugin/releases/download/v2.9.6/samp-streamer-plugin-2.9.6.zip'
    sscanf   = 'https://github.com/Y-Less/sscanf/releases/download/v2.13.8/sscanf-2.13.8-win32.zip'
    pawn     = 'https://github.com/pawn-lang/compiler/releases/download/v3.10.10/pawnc-3.10.10-windows.zip'
}

function Fetch([string]$Url, [string]$Out) {
    Write-Host ("  -> " + (Split-Path -Leaf $Out))
    try {
        Invoke-WebRequest -Uri $Url -OutFile $Out -UseBasicParsing
    } catch {
        throw ("ვერ ჩამოვწერე: " + $Url + "`n" + $_.Exception.Message)
    }
}

function Expand-To([string]$Archive, [string]$Dest) {
    New-Item -ItemType Directory -Force -Path $Dest | Out-Null
    if ($Archive -like '*.zip') {
        Expand-Archive -Path $Archive -DestinationPath $Dest -Force
    } else {
        # .tar.gz -> Windows 10+ has bsdtar
        & tar.exe -xzf $Archive -C $Dest
        if ($LASTEXITCODE -ne 0) { throw "tar ვერ გახსნა არქივი: $Archive" }
    }
}

function Install-File([string]$Dir, [string]$Pattern, [string]$Target) {
    $found = Get-ChildItem -Path $Dir -Recurse -Filter $Pattern -File |
             Where-Object { $_.Extension -in '.dll', '.so' } | Select-Object -First 1
    if (-not $found) { throw ("არქივში ვერ ვიპოვე " + $Pattern) }
    Copy-Item $found.FullName (Join-Path $Plugins $Target) -Force
    Write-Host ("     plugins\" + $Target + "  OK")
}

Write-Host '=============================================================='
Write-Host ' SA-MP Freeroam - dependency downloader (Windows)'
Write-Host '=============================================================='

try {
    # ------------------------------------------------------------- mysql
    Write-Host '[1/3] MySQL plugin (BlueG R41-4)'
    if (Test-Path (Join-Path $Plugins 'mysql.dll')) {
        Write-Host '     plugins\mysql.dll უკვე არსებობს - გამოტოვებულია'
    } else {
        $z = Join-Path $Tmp 'mysql.zip'
        Fetch $Urls.mysql $z
        Expand-To $z (Join-Path $Tmp 'mysql')
        Install-File (Join-Path $Tmp 'mysql') 'mysql.dll' 'mysql.dll'
        # libmysql.dll უნდა იდოს სერვერის ძირეულ საქაღალდეში
        $lib = Get-ChildItem -Path (Join-Path $Tmp 'mysql') -Recurse -Filter 'libmysql.dll' -File |
               Select-Object -First 1
        if ($lib) { Copy-Item $lib.FullName (Join-Path $Root 'libmysql.dll') -Force
                    Write-Host '     libmysql.dll -> სერვერის ძირი  OK' }
    }

    # ---------------------------------------------------------- streamer
    Write-Host '[2/3] Streamer plugin (2.9.6)'
    if (Test-Path (Join-Path $Plugins 'streamer.dll')) {
        Write-Host '     plugins\streamer.dll უკვე არსებობს - გამოტოვებულია'
    } else {
        $z = Join-Path $Tmp 'streamer.zip'
        Fetch $Urls.streamer $z
        Expand-To $z (Join-Path $Tmp 'streamer')
        Install-File (Join-Path $Tmp 'streamer') 'streamer.dll' 'streamer.dll'
    }

    # ------------------------------------------------------------ sscanf
    Write-Host '[3/3] sscanf plugin (2.13.8)'
    if (Test-Path (Join-Path $Plugins 'sscanf.dll')) {
        Write-Host '     plugins\sscanf.dll უკვე არსებობს - გამოტოვებულია'
    } else {
        $z = Join-Path $Tmp 'sscanf.zip'
        Fetch $Urls.sscanf $z
        Expand-To $z (Join-Path $Tmp 'sscanf')
        Install-File (Join-Path $Tmp 'sscanf') 'sscanf.dll' 'sscanf.dll'
    }

    # ------------------------------------------------------ pawn compiler
    $PawnDir = Join-Path $Root 'pawno'
    if (Test-Path (Join-Path $PawnDir 'pawncc.exe')) {
        Write-Host '[+] კომპილატორი: pawno\pawncc.exe უკვე არსებობს'
    } else {
        Write-Host '[+] კომპილატორი: ვაყენებ pawno\pawncc.exe-ს'
        $z = Join-Path $Tmp 'pawnc.zip'
        Fetch $Urls.pawn $z
        $ex = Join-Path $Tmp 'pawnc'
        Expand-To $z $ex
        New-Item -ItemType Directory -Force -Path (Join-Path $PawnDir 'include') | Out-Null

        Get-ChildItem -Path $ex -Recurse -File -Include '*.exe', '*.dll' |
            ForEach-Object { Copy-Item $_.FullName (Join-Path $PawnDir $_.Name) -Force }
        # compiler's own standard includes (float.inc, string.inc, ...)
        Get-ChildItem -Path $ex -Recurse -File -Include '*.inc' |
            ForEach-Object {
                $dest = Join-Path (Join-Path $PawnDir 'include') $_.Name
                if (-not (Test-Path $dest)) { Copy-Item $_.FullName $dest -Force }
            }
    }

    # -------------------------------------------------------- validation
    Write-Host ''
    Write-Host 'შემოწმება:'
    $missing = 0
    foreach ($f in 'mysql.dll', 'streamer.dll', 'sscanf.dll') {
        $p = Join-Path $Plugins $f
        if (Test-Path $p) {
            Write-Host ('  [OK] plugins\' + $f + '  (' + (Get-Item $p).Length + ' ბაიტი)')
        } else {
            Write-Host ('  [X]  plugins\' + $f + '  აკლია')
            $missing++
        }
    }
    if (Test-Path (Join-Path $Root 'libmysql.dll')) {
        Write-Host '  [OK] libmysql.dll (სერვერის ძირში)'
    } else {
        Write-Host '  [!]  libmysql.dll ვერ მოიძებნა - mysql plugin-მა შეიძლება ვერ იმუშაოს'
    }

    Write-Host ''
    if ($missing -eq 0) {
        Write-Host 'ყველაფერი მზადაა: plugins\ სავსეა.'
        Write-Host 'შემდეგი ნაბიჯი:  setup.bat'
    } else {
        throw 'რაღაც აკლია - ნახე შეტყობინებები ზემოთ.' 
    }
}
finally {
    Remove-Item -Recurse -Force $Tmp -ErrorAction SilentlyContinue
}
