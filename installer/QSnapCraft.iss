#ifndef MyAppVersion
  #define MyAppVersion "2.0.0"
#endif
#ifndef SourceDir
  #error SourceDir must be provided by package-release.ps1
#endif
#ifndef OutputDir
  #error OutputDir must be provided by package-release.ps1
#endif
#ifndef RepoRoot
  #error RepoRoot must be provided by package-release.ps1
#endif

#define MyAppName "QSnapCraft"
#define MyAppExeName "QSnapCraft.exe"

[Setup]
AppId={{A3F287F5-6EA8-4B7C-B9D3-EA7F0B9A5C11}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher=QSnapCraft
DefaultDirName={localappdata}\Programs\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
AllowNoIcons=yes
UsePreviousAppDir=yes
LicenseFile={#RepoRoot}\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename=QSnapCraft-{#MyAppVersion}-win64-setup
SetupIconFile={#RepoRoot}\resources\icons\QSnapCraft.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
RestartApplications=no
SetupLogging=yes
SignedUninstaller=no
VersionInfoVersion={#MyAppVersion}.0
VersionInfoDescription={#MyAppName} installer
VersionInfoProductName={#MyAppName}
VersionInfoCompany=QSnapCraft
VersionInfoCopyright=Copyright (c) 2024 LeoLei

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Microsoft Visual C++ Runtime..."; Flags: waituntilterminated; Check: NeedsVCRedist
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\config.ini"
Type: dirifempty; Name: "{app}"

[Code]
function NeedsVCRedist: Boolean;
begin
  Result := (not FileExists(ExpandConstant('{sys}\vcruntime140.dll'))) or
            (not FileExists(ExpandConstant('{sys}\vcruntime140_1.dll')));
end;
