; MB-Studio Windows installer (Inno Setup 6).
;
; Packages the self-contained installation tree produced by
;   cmake --install <build> --prefix <stage>
; (see cmake/MBWindowsDeploy.cmake) into a single setup program.
;
; Build with:
;   iscc /DAppVersion=0.3.1 /DStageDir=C:\path\to\stage /DSourceDir=C:\path\to\MB-Studio
;        /DOutputDir=C:\path\to\dist packaging\windows\mbstudio.iss

#ifndef AppVersion
  #define AppVersion "0.3.1"
#endif
#ifndef StageDir
  #define StageDir "..\..\stage"
#endif
#ifndef SourceDir
  #define SourceDir "..\.."
#endif
#ifndef OutputDir
  #define OutputDir "..\..\dist"
#endif

#define AppName "MB-Studio"
#define AppPublisher "MB-Studio contributors"
#define AppURL "https://github.com/Sqhh99/MB-Studio"
#define AppExe "mbstudio.exe"

[Setup]
AppId={{53D498BC-AA81-4C88-B4D7-B3E1C0C213FE}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}/issues
AppUpdatesURL={#AppURL}/releases
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
LicenseFile={#SourceDir}\packaging\windows\LICENSE.txt
OutputDir={#OutputDir}
OutputBaseFilename=MB-Studio-{#AppVersion}-windows-x64-setup
SetupIconFile={#SourceDir}\packaging\windows\mbstudio.ico
UninstallDisplayIcon={app}\bin\{#AppExe}
UninstallDisplayName={#AppName} {#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
; Per-user or per-machine installation, chosen by the user.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ChangesEnvironment=yes
VersionInfoVersion={#AppVersion}.0
VersionInfoProductName={#AppName}
VersionInfoDescription={#AppName} Setup

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "chinesesimplified"; MessagesFile: "{#SourceDir}\packaging\windows\ChineseSimplified.isl"

[CustomMessages]
english.AddToPath=Add the MB-System programs to the PATH (use them from any console)
japanese.AddToPath=MB-System プログラムを PATH に追加する (任意のコンソールから使用可能)
chinesesimplified.AddToPath=将 MB-System 命令行程序添加到 PATH（可在任意控制台中使用）
english.ConsoleShortcut=MB-System Command Prompt
japanese.ConsoleShortcut=MB-System コマンドプロンプト
chinesesimplified.ConsoleShortcut=MB-System 命令提示符
english.Documentation=MB-System Documentation
japanese.Documentation=MB-System ドキュメント
chinesesimplified.Documentation=MB-System 文档
english.ProgramHelp=MB-System program help
japanese.ProgramHelp=MB-System プログラム ヘルプ
chinesesimplified.ProgramHelp=MB-System 程序帮助

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "addtopath"; Description: "{cm:AddToPath}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; The application tree: programs and DLLs (bin), plugins (lib\mbstudio),
; run-time data (share). Headers and import libraries are not needed.
Source: "{#StageDir}\bin\*"; DestDir: "{app}\bin"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.dll.a,*.a,*.lib,*.pdb"
Source: "{#StageDir}\lib\mbstudio\*"; DestDir: "{app}\lib\mbstudio"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.dll.a,*.a"
Source: "{#StageDir}\share\*"; DestDir: "{app}\share"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\COPYING.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\GPL.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\packaging\windows\mbstudio-cmd.bat"; DestDir: "{app}\bin"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\bin\{#AppExe}"; WorkingDir: "{userdocs}"
Name: "{group}\{cm:ConsoleShortcut}"; Filename: "{cmd}"; Parameters: "/k ""{app}\bin\mbstudio-cmd.bat"""; WorkingDir: "{userdocs}"; IconFilename: "{app}\bin\{#AppExe}"
Name: "{group}\{cm:ProgramHelp}"; Filename: "{app}\share\doc\mbstudio\help\index.html"
Name: "{group}\{cm:Documentation}"; Filename: "{app}\share\doc\mbsystem\html\mbsystem.html"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\bin\{#AppExe}"; WorkingDir: "{userdocs}"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

[Code]
// PATH handling for the "addtopath" task: the user PATH for per-user
// installations, the system PATH for per-machine installations.

function EnvironmentRoot: Integer;
begin
  if IsAdminInstallMode then
    Result := HKEY_LOCAL_MACHINE
  else
    Result := HKEY_CURRENT_USER;
end;

function EnvironmentKey: string;
begin
  if IsAdminInstallMode then
    Result := 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment'
  else
    Result := 'Environment';
end;

function PathContains(const Paths, Dir: string): Boolean;
begin
  Result := Pos(';' + Uppercase(Dir) + ';', ';' + Uppercase(Paths) + ';') > 0;
end;

procedure AddToPath(const Dir: string);
var
  Paths: string;
begin
  if not RegQueryStringValue(EnvironmentRoot, EnvironmentKey, 'Path', Paths) then
    Paths := '';
  if PathContains(Paths, Dir) then
    exit;
  if (Paths <> '') and (Copy(Paths, Length(Paths), 1) <> ';') then
    Paths := Paths + ';';
  RegWriteExpandStringValue(EnvironmentRoot, EnvironmentKey, 'Path', Paths + Dir);
end;

procedure RemoveFromPath(const Dir: string);
var
  Paths: string;
  P: Integer;
begin
  if not RegQueryStringValue(EnvironmentRoot, EnvironmentKey, 'Path', Paths) then
    exit;
  Paths := ';' + Paths + ';';
  P := Pos(';' + Uppercase(Dir) + ';', Uppercase(Paths));
  if P = 0 then
    exit;
  Delete(Paths, P, Length(Dir) + 1);
  Paths := Copy(Paths, 2, Length(Paths) - 2);
  RegWriteExpandStringValue(EnvironmentRoot, EnvironmentKey, 'Path', Paths);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('addtopath') then
    AddToPath(ExpandConstant('{app}\bin'));
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    RemoveFromPath(ExpandConstant('{app}\bin'));
end;
