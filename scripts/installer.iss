[Setup]
AppName=Rampyaaryan
AppVersion=3.5.0
AppPublisher=Rampyaaryans
AppPublisherURL=https://github.com/Rampyaaryans/rampyaaryan
AppSupportURL=https://github.com/Rampyaaryans/rampyaaryan/issues
DefaultDirName={autopf}\Rampyaaryan
DefaultGroupName=Rampyaaryan
OutputDir=..\installer-output
OutputBaseFilename=Rampyaaryan
SetupIconFile=icon.ico
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesEnvironment=yes
ChangesAssociations=yes
PrivilegesRequired=admin
LicenseFile=..\LICENSE
UninstallDisplayIcon={app}\icon.ico

[Files]
Source: "..\rampyaaryan.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "icon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Rampyaaryan"; Filename: "{app}\rampyaaryan.exe"; IconFilename: "{app}\icon.ico"
Name: "{group}\Uninstall Rampyaaryan"; Filename: "{uninstallexe}"

[Registry]
; PATH
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; \
    ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; \
    Check: NeedsAddPath('{app}')
; .ram file association
Root: HKCR; Subkey: ".ram"; ValueType: string; ValueData: "RampyaaryanFile"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "RampyaaryanFile"; ValueType: string; ValueData: "Rampyaaryan Source File"; Flags: uninsdeletekey
Root: HKCR; Subkey: "RampyaaryanFile\DefaultIcon"; ValueType: string; ValueData: "{app}\icon.ico"
Root: HKCR; Subkey: "RampyaaryanFile\shell\open\command"; ValueType: string; ValueData: """{app}\rampyaaryan.exe"" ""%1"""
Root: HKCR; Subkey: "RampyaaryanFile\shell\Run with Rampyaaryan"; ValueType: string; ValueData: "Run with Rampyaaryan"
Root: HKCR; Subkey: "RampyaaryanFile\shell\Run with Rampyaaryan\command"; ValueType: string; ValueData: """{app}\rampyaaryan.exe"" ""%1"""

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
