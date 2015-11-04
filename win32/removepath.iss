; Script to remove a path from the PATH environment

#include "ModifyPath.iss"

[Setup]
AppName=removepath
AppVerName=removepath 1.0
CreateAppDir=false
DisableStartupPrompt=true
OutputBaseFilename=removepath
OutputDir=.

[Code]
function InitializeSetup(): boolean;
var
  lparam : String;
  scope  : Integer;
begin
  { The actual parameters to the setup executable start at number 6 }
  { We expect 2 parameters }
  if ParamCount = 7 then
  begin
    scope := psAllUsers;
    if StrToInt(ParamStr(7)) = 2 then
    begin
      scope := psCurrentUser
    end
    ModifyPath(ParamStr(6), pmRemove, scope);
    { SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment") }
    lparam := 'Environment';
    SendMessage($FFFF, $001A, 0, CastStringToInteger(lparam));
  end
  Result := False;
end;

