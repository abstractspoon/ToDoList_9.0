ECHO OFF
CLS

pushd %~dp0
set REPO=%CD%
ECHO REPO=%REPO%

if NOT EXIST %REPO%\Core exit
if NOT EXIST %REPO%\Plugins exit

ECHO ON

REM - Make sure caller has updated ToDoList version number
pause

REM - Build core app in VC6
cd %REPO%\Core
"C:\Program Files (x86)\Microsoft Visual Studio\Common\MSDev98\Bin\msdev.exe" .\ToDoList_Core.dsw /MAKE "ALL - Win32 Unicode Release" 

REM - Copy ToDoList.pdb/.map to versioned symbols folder
%REPO%\Core\ToDoList\Unicode_Release\ToDoList.exe -ver
SET /P TDLVER=< .\ver.txt
DEL .\ver.txt

MKDIR %REPO%\..\ToDoList_Symbols
MKDIR %REPO%\..\ToDoList_Symbols\%TDLVER%

COPY /Y /B %REPO%\Core\ToDoList\Unicode_Release\ToDoList.pdb %REPO%\..\ToDoList_Symbols\%TDLVER%\
COPY /Y /B %REPO%\Core\ToDoList\Unicode_Release\ToDoList.map %REPO%\..\ToDoList_Symbols\%TDLVER%\

SET MSBUILD="C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\MSBuild\15.0\Bin\MSBuild.exe"
SET BUILDPARAMS=/t:Build /p:Configuration=Release /v:normal /noWarn:MSB3267;MSB3305

cd %REPO%\Plugins

REM - Build ToDoList_Core\3rdParty and ToDoList_Core\Shared in VS2010 for linking into PluginsHelpers
%MSBUILD% .\Core\ToDoList_Core_For_Plugins.sln %BUILDPARAMS%

REM - Build rest of plugins
%MSBUILD% .\ToDoList_Plugins.sln %BUILDPARAMS%

REM - Allow caller to cancel building Zip
pause

CALL %REPO%\BuildReleaseZip.bat

popd
