[Setup]
AppName=Rampyaaryan
AppVersion=3.2.0
AppPublisher=Rampyaaryans
AppPublisherURL=https://github.com/Rampyaaryans/rampyaaryan
AppSupportURL=https://github.com/Rampyaaryans/rampyaaryan/issues
DefaultDirName={autopf}\Rampyaaryan
DefaultGroupName=Rampyaaryan
OutputDir=..\installer-output
OutputBaseFilename=rampyaaryan-windows-x64-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesEnvironment=yes
PrivilegesRequired=admin
LicenseFile=..\LICENSE
UninstallDisplayIcon={app}\rampyaaryan.exe

[Files]
Source: "..\rampyaaryan.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Rampyaaryan"; Filename: "{app}\rampyaaryan.exe"
Name: "{group}\Uninstall Rampyaaryan"; Filename: "{uninstallexe}"

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; \
    ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; \
    Check: NeedsAddPath('{app}')

[Code]
function NeedsAddPath(Param: string): Boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
    'Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  Result := Pos(';' + UpperCase(Param) + ';', ';' + UpperCase(OrigPath) + ';') = 0;
end;
