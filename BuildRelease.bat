title ToDoList_9.0 Release Build

ECHO OFF
CLS

pushd %~dp0
set REPO=%CD%
ECHO REPO=%REPO%

if NOT EXIST %REPO%\Core exit
if NOT EXIST %REPO%\Plugins exit

ECHO ON

REM - Remember to update ToDoList version number
REM - Remember to pull latest translations
pause

REM - Build core app in VC6
cd %REPO%\Core

REM - Detours
REM "C:\Program Files (x86)\Microsoft Visual Studio\Common\MSDev98\Bin\msdev.exe" .\3rdParty\Detours\Detours.dsw /MAKE "ALL - Win32 Unicode Release"

REM - Core app
set OUTPUT_FILE=%REPO%\Core\ToDoList\Unicode_Release\Core_Build_Output.txt
"C:\Program Files (x86)\Microsoft Visual Studio\Common\MSDev98\Bin\msdev.exe" .\ToDoList_Core.dsw /MAKE "ALL - Win32 Unicode Release" /OUT %OUTPUT_FILE% 

REM - Check for compile errors
ECHO OFF

findstr /C:") : error" %OUTPUT_FILE%
if %errorlevel%==1 (
REM - Check for link errors
findstr /C:"Error executing link.exe" %OUTPUT_FILE%
)

if %errorlevel%==1 (
echo [42m Build SUCCEEDED[0m
)
if %errorlevel%==0 (
echo [41m Build FAILED[0m
pause
exit
)

REM Run units tests
ECHO ON

cd TDLTest\Unicode_Release

set OUTPUT_FILE=%REPO%\Core\TDLTest\Unicode_Release\Test_Output.txt
del %OUTPUT_FILE%

TDLTest > %OUTPUT_FILE%

REM - Check for test errors
ECHO OFF
findstr /C:"tests FAILED" %OUTPUT_FILE%

if %errorlevel%==0 (
echo [41m Tests FAILED[0m
pause
exit
)

REM - Check for test success
findstr /C:"tests SUCCEEDED" %OUTPUT_FILE% > nul

if %errorlevel%==0 (
echo [42m Tests SUCCEEDED[0m
)
if %errorlevel%==1 (
echo [41m Test Results EMPTY[0m
pause
exit
)

REM - Build plugins using MSBuild for reliability
ECHO ON

SET MSBUILD="C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe"
cd %REPO%\Plugins

set OUTPUT_FILE=%REPO%\Plugins\Release\Build_Output.txt
del %OUTPUT_FILE%

%MSBUILD% .\ToDoList_Plugins.sln /t:Build /p:Configuration=Release /m /v:normal > %OUTPUT_FILE%

REM - Check for build errors
ECHO OFF
findstr /C:"Build FAILED." %OUTPUT_FILE%

if %errorlevel%==1 (
echo [42m Build SUCCEEDED[0m
)
if %errorlevel%==0 (
echo [41m Build FAILED[0m
pause
exit
)

ECHO ON

REM - Allow caller to cancel building Zip
pause

CALL %REPO%\BuildReleaseZip.bat
CALL %REPO%\BuildCodeZip.bat

REM - Open Downloads folder
Explorer.exe "%REPO%\..\ToDoList_Downloads\Latest"

popd
pause
