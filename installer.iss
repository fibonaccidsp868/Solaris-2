; Inno Setup script - builds Solaris2-Setup.exe (one-click Windows installer)
[Setup]
AppName=Solaris 2
AppVersion=2.0.0
AppPublisher=Fibonacci
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=Solaris2-Setup
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64
Compression=lzma2
SolidCompression=yes

[Files]
Source: "build\Solaris2_artefacts\Release\VST3\Solaris 2.vst3\*"; DestDir: "{commoncf64}\VST3\Solaris 2.vst3"; Flags: recursesubdirs ignoreversion
