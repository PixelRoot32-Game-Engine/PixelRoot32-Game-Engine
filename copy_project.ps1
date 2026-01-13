Write-Host "Copying ESP32 Game Engine to Arduino libraries..."

# Define source path (current project directory)
$SRC = Get-Location

# Detect if OneDrive path exists (Office PC)
if (Test-Path "$env:USERPROFILE\Documents\Arduino\libraries\") {
    $DEST = "$env:USERPROFILE\Documents\Arduino\libraries\ESP32 Game Engine"
}
# Otherwise, use default home path
else {
    $DEST = "$env:OneDrive\Documents 1\Arduino\libraries\ESP32 Game Engine"
}

Write-Host "Using destination path: $DEST"

# Ensure destination exists
If (!(Test-Path $DEST)) {
    New-Item -ItemType Directory -Path $DEST -Force
}

# Copy all files while preserving structure
Copy-Item -Path "$SRC\*" -Destination "$DEST\" -Recurse -Force

Write-Host "Done!"
