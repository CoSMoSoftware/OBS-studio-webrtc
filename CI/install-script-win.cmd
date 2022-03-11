REM if exist dependencies2019.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/dependencies2019.zip -f --retry 5 dependencies2019.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/dependencies2019.zip -f --retry 5 -C -)
REM if exist vlc.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/vlc.zip -f --retry 5 vlc.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/vlc.zip -f --retry 5 -C -)
REM if exist cef_binary_%CEF_VERSION%_windows_x64.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows_x64.zip -f --retry 5 cef_binary_%CEF_VERSION%_windows_x64.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows_x64.zip -f --retry 5 -C -)
REM 7z x dependencies2019.zip -odependencies2019
REM 7z x vlc.zip -ovlc
REM 7z x cef_binary_%CEF_VERSION%_windows_x64.zip -oCEF_64
set DepsPath64=%CD%\dependencies2019\win64
set VLCPath=%CD%\vlc
set QTDIR64=C:\QtDep\Qt\5.15.2\msvc2019_64
set CEF_64=%CD%\CEF_64\cef_binary_%CEF_VERSION%_windows_x64
set build_config=RelWithDebInfo
set VIRTUALCAM-GUID=A3FCE0F5-3493-419F-958A-ABA1250EC20B
git stash
REM Parameter %1 = vendor name
if "%1"=="Millicast" (
  set vendor_option=
) else (
  set vendor_option="-DOBS_WEBRTC_VENDOR_NAME=%1"
)
set "_remote_filming="
if "%1"=="RemoteFilming" set "_remote_filming=1"
if "%1"=="RemoteFilming-A" set "_remote_filming=1"
if "%1"=="RemoteFilming-B" set "_remote_filming=1"
if "%1"=="RemoteFilming-C" set "_remote_filming=1"
if "%1"=="RemoteFilming-D" set "_remote_filming=1"
if "%_remote_filming%"=="1" (
  copy .\vendor_skins\%1\CMakeLists.txt .\CMakeLists.txt
  copy .\vendor_skins\%1\UI\ui-config.h.in .\UI\ui-config.h.in
)
mkdir build64_%1
cd build64_%1
cmake %vendor_option% -G "Visual Studio 16 2019" -A x64 -DBUILD_NDI=ON -DBUILD_WEBSOCKET=ON -DLibObs_DIR=%CD%\libobs -DLIBOBS_LIB=%CD%\libobs\Release\obs.lib -Dw32-pthreads_DIR=%CD%\deps\w32-pthreads -DOBS_FRONTEND_LIB=%CD%\UI\obs-frontend-api\Release\obs-frontend-api.lib -DLIBOBS_INCLUDE_DIR=%CD%\..\libobs -DQt5_DIR=%QTDIR64%\lib\cmake\Qt5 -DCMAKE_SYSTEM_VERSION=10.0 -DOBS_VERSION_OVERRIDE=%OBS_VERSION% -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DENABLE_VLC=ON -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_64% -Dlibwebrtc_DIR="%CD%\libwebrtc\cmake" -DOPENSSL_ROOT_DIR="%CD%\..\openssl-1.1\x64" -DVIRTUALCAM_GUID="%VIRTUALCAM-GUID%" -DQt5Widgets_DIR=%QTDIR64%\lib\cmake\Qt5Widgets -DQt5Svg_DIR=%QTDIR64%\lib\cmake\Qt5Svg -DQt5Xml_DIR=%QTDIR64%\lib\cmake\Qt5Xml ..
if "%1"=="Millicast" (
  move obs-studio.sln Millicast.sln
)
cd ..
