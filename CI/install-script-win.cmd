set DepsPath64=%CD%\dependencies2019\win64
set VLCPath=%CD%\vlc
set QTDIR64=C:\QtDep\5.10.1\msvc2017_64
set CEF_64=%CD%\CEF_64\cef_binary_%CEF_VERSION%_windows64_minimal
set libwebrtcPath=%CD%\libwebrtc\cmake
set opensslPath=%CD%\openssl-1.1\x64
git stash
REM Parameter %1 = vendor name
if "%1"=="Millicast" (
  set vendor_option=
) else (
  set vendor_option="-DOBS_WEBRTC_VENDOR_NAME=%1"
)
mkdir build_%1 build32_%1 build64_%1
cd build64_%1
if "%TWITCH-CLIENTID%"=="$(twitch_clientid)" (
  cmake %vendor_option% -G "Visual Studio 16 2019" -A x64 -DBUILD_NDI=ON -DBUILD_WEBSOCKET=ON -DLibObs_DIR=%CD%\libobs -DLIBOBS_LIB=%CD%\libobs\Release\obs.lib -Dw32-pthreads_DIR=%CD%\deps\w32-pthreads -DOBS_FRONTEND_LIB=%CD%\UI\obs-frontend-api\Release\obs-frontend-api.lib -DLIBOBS_INCLUDE_DIR=%CD%\..\libobs -DQt5_DIR=%QTDIR64%\lib\cmake\Qt5 -DCMAKE_SYSTEM_VERSION=10.0 -DOBS_VERSION_OVERRIDE=%OBS_VERSION% -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DENABLE_VLC=ON -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_64% -Dlibwebrtc_DIR="%CD%\libwebrtc\cmake" -DOPENSSL_ROOT_DIR="%CD%\..\openssl-1.1\x64" ..
) else (
  cmake %vendor_option% -G "Visual Studio 16 2019" -A x64 -DBUILD_NDI=ON -DBUILD_WEBSOCKET=ON -DLibObs_DIR=%CD%\libobs -DLIBOBS_LIB=%CD%\libobs\Release\obs.lib -Dw32-pthreads_DIR=%CD%\deps\w32-pthreads -DOBS_FRONTEND_LIB=%CD%\UI\obs-frontend-api\Release\obs-frontend-api.lib -DLIBOBS_INCLUDE_DIR=%CD%\..\libobs -DQt5_DIR=%QTDIR64%\lib\cmake\Qt5 -DCMAKE_SYSTEM_VERSION=10.0 -DOBS_VERSION_OVERRIDE=%OBS_VERSION% -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DENABLE_VLC=ON -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_64% -Dlibwebrtc_DIR="%CD%\libwebrtc\cmake" -DOPENSSL_ROOT_DIR="%CD%\..\openssl-1.1\x64" -DTWITCH_CLIENTID="%TWITCH-CLIENTID%" -DTWITCH_HASH="%TWITCH-HASH%"  -DRESTREAM_CLIENTID="%RESTREAM-CLIENTID%" -DRESTREAM_HASH="%RESTREAM-HASH%" ..
)
if "%1"=="Millicast" (
  move obs-studio.sln Millicast.sln
)
cd ..
