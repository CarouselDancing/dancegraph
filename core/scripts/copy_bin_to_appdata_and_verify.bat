@echo off

set sourcedir=%cd%\DanceGraphAppData
set sourcemodulesdir=%cd%\DanceGraphAppData\modules

set destdir=%LOCALAPPDATA%\DanceGraph
set destmodulesdir=%LOCALAPPDATA%\DanceGraph\modules

set cleandest=%destdir:"=%
set cleanmodulesdir=%destmodulesdir:"=%

if not exist %cleandest% mkdir %cleandest%
if not exist %cleanmodulesdir% mkdir %cleanmodulesdir%

echo Copying and verifying

pushd %sourcedir%
for %%f in (*) do (
    copy "%%f" "%cleandest%\%%f" /Y 
    echo "%%f" "%cleandest%\%%~f" 
    fc "%%f" "%cleandest%\%%~f" > nul
    if errorlevel 1 goto :fail %%f,%cleandest%\%%f
    if errorlevel 2 goto :fail %%f,%cleandest%\%%f
    )
    
    pushd modules
    for %%f in (*) do (
    copy "%%f" "%cleanmodulesdir%\%%f" /Y 
    fc "%%f" "%cleanmodulesdir%\%%f" > nul
    if errorlevel 1 goto :fail %%f,%cleanmodulesdir%\%%f
    if errorlevel 2 goto :fail %%f,%cleanmodulesdir%\%%f
    )
    popd
popd
echo Files verified

EXIT /B 0

:fail
echo "ERROR: Failed copying file %~1 -> %~2, please rectify and rerun"
pause
EXIT /B 1


