; OLIVERB Windows installer — built by CI with Inno Setup 6.
; Installs the VST3 bundle into the standard system VST3 folder.

[Setup]
AppName=OLIVERB
AppVersion=1.0.0
AppPublisher=Deestech
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=OLIVERB-Windows-Setup
OutputDir=..\..\dist
Compression=lzma2
SolidCompression=yes
UninstallFilesDir={commoncf64}\VST3\OLIVERB.vst3

[Files]
Source: "..\..\dist\OLIVERB.vst3\*"; DestDir: "{commoncf64}\VST3\OLIVERB.vst3"; Flags: recursesubdirs ignoreversion
