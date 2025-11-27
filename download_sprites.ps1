# Download Pokemon sprites from PokeAPI
# Downloads all Pokemon from Gen 1-9 (Species 1-1025)
# Includes normal and shiny variants

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "Pokemon Sprite Downloader" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "Downloading ALL Pokemon sprites (Gen 1-9)"
Write-Host "Species: 1-1025"
Write-Host ""

# Create directories
New-Item -ItemType Directory -Force -Path "romfs\sprites\pokemon" | Out-Null
New-Item -ItemType Directory -Force -Path "romfs\sprites\icons" | Out-Null

Set-Location "romfs\sprites\pokemon"

# Download all Pokemon (Gen 1-9)
$START_ID = 1
$END_ID = 1025
$SUCCESS = 0
$FAILED = 0
$TOTAL = 1025

Write-Host "Starting download..."
Write-Host ""

for ($i = $START_ID; $i -le $END_ID; $i++) {
    # Progress indicator every 50 Pokemon
    if ($i % 50 -eq 0) {
        $PERCENT = [math]::Floor($i * 100 / $TOTAL)
        Write-Host "Progress: $PERCENT% ($i/$END_ID) - Success: $SUCCESS, Failed: $FAILED" -ForegroundColor Yellow
    }

    # Download normal sprite
    $url = "https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/$i.png"
    try {
        Invoke-WebRequest -Uri $url -OutFile "$i.png" -ErrorAction Stop | Out-Null
        $SUCCESS++
    } catch {
        $FAILED++
    }

    # Download shiny sprite
    $shinyUrl = "https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/shiny/$i.png"
    try {
        Invoke-WebRequest -Uri $shinyUrl -OutFile "${i}s.png" -ErrorAction Stop | Out-Null
        $SUCCESS++
    } catch {
        $FAILED++
    }

    # Rate limit: pause every 20 Pokemon
    if ($i % 20 -eq 0) {
        Start-Sleep -Milliseconds 500
    }
}

Set-Location "..\..\..\"

Write-Host ""
Write-Host "=========================================" -ForegroundColor Green
Write-Host "Download Complete!" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
Write-Host "Total sprites downloaded: $SUCCESS"
Write-Host "Failed downloads: $FAILED"
Write-Host "Sprites saved to: romfs\sprites\pokemon\"
Write-Host ""
Write-Host "Generation breakdown:"
Write-Host "  Gen 1: 1-151      (Kanto)"
Write-Host "  Gen 2: 152-251    (Johto)"
Write-Host "  Gen 3: 252-386    (Hoenn)"
Write-Host "  Gen 4: 387-493    (Sinnoh)"
Write-Host "  Gen 5: 494-649    (Unova)"
Write-Host "  Gen 6: 650-721    (Kalos)"
Write-Host "  Gen 7: 722-809    (Alola)"
Write-Host "  Gen 8: 810-905    (Galar)"
Write-Host "  Gen 9: 906-1025   (Paldea)"
Write-Host ""
Write-Host "Ready to build with 'make'!" -ForegroundColor Green
