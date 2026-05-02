# Link verification script
$docsRoot = "C:\Users\gperez88\Documents\Proyects\Games\pixelroot32 workspace\PixelRoot32-Game-Samples\lib\PixelRoot32-Game-Engine\docs"

$mdFiles = Get-ChildItem -Path $docsRoot -Recurse -Filter "*.md"
$brokenLinks = @()
$totalLinks = 0

foreach ($file in $mdFiles) {
    $lines = Get-Content $file.FullName
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '\]\(([^)]+\.md)\)') {
            $links = [regex]::Matches($lines[$i], '\]\(([^)]+\.md)\)')
            foreach ($link in $links) {
                $totalLinks++
                $target = $link.Groups[1].Value
                
                # Resolve relative path
                $fileDir = Split-Path $file.FullName -Parent
                if ($target -match '^[^/]') {
                    $fullTarget = Join-Path $fileDir $target
                } else {
                    $fullTarget = Join-Path $docsRoot $target
                }
                $fullTarget = $fullTarget.Replace('/', '\')
                
                if (-not (Test-Path $fullTarget)) {
                    # Also check for common redirects/merges
                    $baseName = Split-Path $target -Leaf
                    $baseNameNoExt = [System.IO.Path]::GetFileNameWithoutExtension($baseName)
                    
                    # Skip known redirects that were merged
                    $knownRedirects = @('overview', 'layers-overview', 'index', 'style-guide', 'tilemaps', 'multi-palette', 'platform-config', 'resolution-scaling')
                    if ($knownRedirects -contains $baseNameNoExt) {
                        continue  # Skip known redirects
                    }
                    
                    $brokenLinks += [PSCustomObject]@{
                        File = $file.Name
                        Line = $i + 1
                        Link = $target
                        Note = "Target not found"
                    }
                }
            }
        }
    }
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Link Verification Report" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Total links checked: $totalLinks" -ForegroundColor Gray
Write-Host "Broken links found: $($brokenLinks.Count)" -ForegroundColor $(if ($brokenLinks.Count -eq 0) { "Green" } else { "Red" })
Write-Host ""

if ($brokenLinks.Count -gt 0) {
    Write-Host "Broken Links:" -ForegroundColor Red
    Write-Host "-" * 80
    $brokenLinks | ForEach-Object {
        Write-Host "  $($_.File):$($_.Line) -> $($_.Link)" -ForegroundColor Yellow
    }
} else {
    Write-Host "No broken links found!" -ForegroundColor Green
}

# Also check for references to deleted/merged files
Write-Host ""
Write-Host "Checking for references to merged files (informational):" -ForegroundColor Cyan

$mergedFiles = @('overview.md', 'layers-overview.md', 'style-guide.md', 'tilemaps.md', 'multi-palette.md', 'platform-config.md', 'resolution-scaling.md')
$mergedRefs = @()

foreach ($file in $mdFiles) {
    $content = Get-Content $file.FullName -Raw
    foreach ($merged in $mergedFiles) {
        if ($content -match [regex]::Escape($merged)) {
            $mergedRefs += [PSCustomObject]@{
                File = $file.Name
                References = $merged
            }
        }
    }
}

if ($mergedRefs.Count -gt 0) {
    Write-Host "Files still referencing merged files:" -ForegroundColor Yellow
    $mergedRefs | ForEach-Object {
        Write-Host "  $($_.File) -> $($_.References)" -ForegroundColor Gray
    }
} else {
    Write-Host "  None" -ForegroundColor Green
}