@echo off
setlocal enableextensions enabledelayedexpansion
path %path%;%windir%\System32;%windir%;%windir%\System32\Wbem;%windir%\System32\WindowsPowerShell\v1.0

:: Unattended install flag. When set, the script will not require user input.
set unattended=no
if "%1"=="/u" set unattended=yes

:: Make sure the script is running as admin
fltmc >nul 2>&1 || (
	if "%unattended%"=="yes" call :die "Admin permissions are required, and aren't present." || exit /b 1

	echo Administrator privileges are required.
	if "%*"=="" (
		echo no args
		powershell Start -Verb RunAs '%~0' 2> nul || goto admin_fail
	) else (
		echo args
		powershell Start -Verb RunAs '%0' -ArgumentList '%*' 2> nul || goto admin_fail
	)
	exit /b 0
)

:: Delete "App Paths" entry
reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\mpc-qt.exe" /f >nul

:: Delete HKCR subkeys
set classes_root_key=HKLM\SOFTWARE\Classes
reg delete "%classes_root_key%\Applications\mpc-qt.exe" /f >nul
reg delete "%classes_root_key%\SystemFileAssociations\video\OpenWithList\mpc-qt.exe" /f >nul
reg delete "%classes_root_key%\SystemFileAssociations\audio\OpenWithList\mpc-qt.exe" /f >nul

:: Delete AutoPlay handlers
set autoplay_key=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AutoplayHandlers
reg delete "%autoplay_key%\Handlers\mpc-qtPlayDVDMovieOnArrival" /f >nul
reg delete "%autoplay_key%\EventHandlers\PlayDVDMovieOnArrival" /v "mpc-qtPlayDVDMovieOnArrival" /f >nul
reg delete "%autoplay_key%\Handlers\mpc-qtPlayBluRayOnArrival" /f >nul
reg delete "%autoplay_key%\EventHandlers\PlayBluRayOnArrival" /v "mpc-qtPlayBluRayOnArrival" /f >nul

:: Delete "Default Programs" entry
reg delete "HKLM\SOFTWARE\RegisteredApplications" /v "MPC-QT" /f >nul
reg delete "HKLM\SOFTWARE\Clients\Media\MPC-QT\Capabilities" /f >nul

:: Delete all OpenWithProgIds referencing ProgIds that start with io.mpc-qt.
for /f "usebackq eol= delims=" %%k in (`reg query "%classes_root_key%" /f "io.mpc-qt.*" /s /v /c`) do (
	set "key=%%k"
	echo !key!| findstr /r /i "^HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\\.[^\\][^\\]*\\OpenWithProgIds$" >nul
	if not errorlevel 1 (
		for /f "usebackq eol= tokens=1" %%v in (`reg query "!key!" /f "io.mpc-qt.*" /v /c`) do (
			set "value=%%v"
			echo !value!| findstr /r /i "^io\.mpc-qt\.[^\\][^\\]*$" >nul
			if not errorlevel 1 (
				echo Deleting !key!\!value!
				reg delete "!key!" /v "!value!" /f >nul
			)
		)
	)
)

:: Delete all ProgIds starting with io.mpc-qt.
for /f "usebackq eol= delims=" %%k in (`reg query "%classes_root_key%" /f "io.mpc-qt.*" /k /c`) do (
	set "key=%%k"
	echo !key!| findstr /r /i "^HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\io\.mpc-qt\.[^\\][^\\]*$" >nul
	if not errorlevel 1 (
		echo Deleting !key!
		reg delete "!key!" /f >nul
	)
)

echo]
echo Uninstalled successfully^^!
if [%unattended%] == [yes] exit /b 0
pause
exit 0

:die
	if not [%1] == [] echo %~1
	if [%unattended%] == [yes] exit 1
	pause
	exit 1

:admin_fail
	echo Failed auto-elevation.
	echo Right-click on mpc-qt-assoc-install.bat and select "Run as administrator".
	call :die || exit /b 1
	goto :EOF
