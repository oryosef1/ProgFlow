# Windows Packaging Script for ProgFlow
# Creates an NSIS installer

param(
    [string]$Version = "1.0.0",
    [string]$SignCert = "",
    [string]$SignPassword = ""
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectDir "build\windows-release"
$OutputDir = Join-Path $ProjectDir "dist"

Write-Host "=== ProgFlow Windows Packaging ===" -ForegroundColor Cyan
Write-Host "Version: $Version"

# Build if needed
if (-not (Test-Path $BuildDir)) {
    Write-Host "Building release..."
    cmake --preset windows-release
    cmake --build --preset windows-release
}

# Create output directory
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

# Paths
$AppExe = Join-Path $BuildDir "ProgFlow_artefacts\Release\ProgFlow.exe"
$PluginVST3 = Join-Path $BuildDir "ProgFlowPlugin_artefacts\Release\VST3\ProgFlow.vst3"

if (-not (Test-Path $AppExe)) {
    Write-Error "App executable not found at $AppExe"
    exit 1
}

# Create installer directory structure
$InstallerTemp = Join-Path $OutputDir "installer-temp"
Remove-Item -Recurse -Force $InstallerTemp -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $InstallerTemp | Out-Null
New-Item -ItemType Directory -Force -Path "$InstallerTemp\app" | Out-Null
New-Item -ItemType Directory -Force -Path "$InstallerTemp\plugins\VST3" | Out-Null

# Copy files
Copy-Item $AppExe "$InstallerTemp\app\"

# Copy DLLs if any
$AppDir = Split-Path -Parent $AppExe
Get-ChildItem -Path $AppDir -Filter "*.dll" | ForEach-Object {
    Copy-Item $_.FullName "$InstallerTemp\app\"
}

# Copy VST3 plugin
if (Test-Path $PluginVST3) {
    Copy-Item -Recurse $PluginVST3 "$InstallerTemp\plugins\VST3\"
}

# Create NSIS script
$NSISScript = @"
!include "MUI2.nsh"

Name "ProgFlow $Version"
OutFile "$OutputDir\ProgFlow-$Version-Windows-Setup.exe"
InstallDir "`$PROGRAMFILES64\ProgFlow"
RequestExecutionLevel admin

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "$ProjectDir\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "ProgFlow" SecMain
    SetOutPath "`$INSTDIR"
    File /r "$InstallerTemp\app\*.*"

    ; Create Start Menu shortcuts
    CreateDirectory "`$SMPROGRAMS\ProgFlow"
    CreateShortcut "`$SMPROGRAMS\ProgFlow\ProgFlow.lnk" "`$INSTDIR\ProgFlow.exe"
    CreateShortcut "`$SMPROGRAMS\ProgFlow\Uninstall.lnk" "`$INSTDIR\Uninstall.exe"

    ; Create Desktop shortcut
    CreateShortcut "`$DESKTOP\ProgFlow.lnk" "`$INSTDIR\ProgFlow.exe"

    ; Write uninstaller
    WriteUninstaller "`$INSTDIR\Uninstall.exe"

    ; Registry entries for uninstall
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ProgFlow" "DisplayName" "ProgFlow"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ProgFlow" "UninstallString" "`$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ProgFlow" "DisplayVersion" "$Version"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ProgFlow" "Publisher" "ProgFlow"
SectionEnd

Section "VST3 Plugin" SecVST3
    SetOutPath "`$COMMONPROGRAMFILES64\VST3"
    File /r "$InstallerTemp\plugins\VST3\*.*"
SectionEnd

Section "Uninstall"
    Delete "`$INSTDIR\*.*"
    RMDir /r "`$INSTDIR"

    Delete "`$SMPROGRAMS\ProgFlow\*.*"
    RMDir "`$SMPROGRAMS\ProgFlow"

    Delete "`$DESKTOP\ProgFlow.lnk"

    Delete "`$COMMONPROGRAMFILES64\VST3\ProgFlow.vst3"
    RMDir /r "`$COMMONPROGRAMFILES64\VST3\ProgFlow.vst3"

    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ProgFlow"
SectionEnd
"@

$NSISScriptPath = Join-Path $InstallerTemp "installer.nsi"
$NSISScript | Out-File -FilePath $NSISScriptPath -Encoding UTF8

Write-Host "Creating installer with NSIS..."

# Run NSIS (assuming it's in PATH)
$nsisPath = "makensis"
if (Get-Command $nsisPath -ErrorAction SilentlyContinue) {
    & $nsisPath $NSISScriptPath
} else {
    Write-Warning "NSIS not found in PATH. Please install NSIS and run: makensis $NSISScriptPath"
    Write-Host "Installer script created at: $NSISScriptPath"
}

# Code signing (if certificate provided)
$InstallerPath = Join-Path $OutputDir "ProgFlow-$Version-Windows-Setup.exe"
if ((Test-Path $InstallerPath) -and $SignCert) {
    Write-Host "Signing installer..."
    & signtool sign /f $SignCert /p $SignPassword /t http://timestamp.digicert.com $InstallerPath
}

# Clean up
Remove-Item -Recurse -Force $InstallerTemp -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "=== Done ===" -ForegroundColor Green
if (Test-Path $InstallerPath) {
    Get-Item $InstallerPath | Format-List Name, Length
}
