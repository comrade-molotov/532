# ============================================================================
#  SA-MP Freeroam 0.3.7 - ერთი ბრძანებით მომზადება (Windows / PowerShell)
#
#  1. ამოწმებს/ჩამოტვირთავს plugin-ებს
#  2. ამოწმებს კომპილატორს (pawno\pawncc.exe)
#  3. აკომპილირებს gamemodes\freeroam.pwn -> gamemodes\freeroam.amx
#  4. ბეჭდავს დარჩენილ ნაბიჯებს
# ============================================================================
$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

Write-Host '=============================================================='
Write-Host ' SA-MP Freeroam 0.3.7 - setup (Windows)'
Write-Host '=============================================================='
Write-Host ''

Write-Host '[1/4] plugin-ები'
& (Join-Path $Root 'download_dependencies.ps1')

foreach ($f in 'mysql.dll', 'streamer.dll', 'sscanf.dll') {
    if (-not (Test-Path (Join-Path $Root ('plugins\' + $f)))) { throw ('plugins\' + $f + ' აკლია') }
}

Write-Host ''
Write-Host '[2/4] კომპილატორი'
$Pawncc = Join-Path $Root 'pawno\pawncc.exe'
if (-not (Test-Path $Pawncc)) { throw 'pawno\pawncc.exe ვერ ვიპოვე' }
Write-Host '  [OK] pawno\pawncc.exe'

Write-Host ''
Write-Host '[3/4] კომპილაცია'
$Out = Join-Path $Root 'gamemodes\freeroam.amx'
if (Test-Path $Out) { Remove-Item $Out -Force }

& $Pawncc 'gamemodes\freeroam.pwn' '-ogamemodes\freeroam' '-iincludes' '-ipawno\include' '-d3' '-(+' '-;+'
$compileOk = Test-Path $Out
if ($compileOk) {
    Write-Host ('  [OK] gamemodes\freeroam.amx (' + (Get-Item $Out).Length + ' ბაიტი)')
} else {
    throw 'კომპილაცია ვერ მოხერხდა - ნახე შეცდომები ზემოთ'
}

Write-Host ''
Write-Host '[4/4] დარჩენილი ნაბიჯები (ერთხელ):'
Write-Host ''
Write-Host '  ა) შექმენი ბაზა (თუ ჯერ არ გაქვს):'
Write-Host '       mysql -u root -p < database.sql'
Write-Host ''
Write-Host '  ბ) gamemodes\freeroam.pwn-ის თავში შეავსე DB_HOST / DB_USER / DB_PASS / DB_NAME'
Write-Host '     და ხელახლა გაუშვი: setup.bat'
Write-Host ''
Write-Host '  გ) server.cfg-ში შეცვალე rcon_password'
Write-Host ''
Write-Host '  დ) გაშვება:'
Write-Host '       samp-server.exe'
Write-Host ''
Write-Host '  დეტალური ინსტრუქცია: README.md'
Write-Host ''
