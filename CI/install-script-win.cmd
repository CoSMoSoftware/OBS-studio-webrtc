set DepsPath64=%CD%\dependencies2017\win64
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
  cmake %vendor_option% -G "Visual Studio 16 2019" -A x64 -DCMAKE_SYSTEM_VERSION=10.0 -DOBS_VERSION_OVERRIDE="26.0.2" -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DENABLE_VLC=ON -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_64% -Dlibwebrtc_DIR="%CD%\libwebrtc\cmake" -DOPENSSL_ROOT_DIR="%CD%\..\openssl-1.1\x64" ..
) else (
  cmake %vendor_option% -G "Visual Studio 16 2019" -A x64 -DCMAKE_SYSTEM_VERSION=10.0 -DOBS_VERSION_OVERRIDE="26.0.2" -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DENABLE_VLC=ON -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_64% -Dlibwebrtc_DIR="%CD%\libwebrtc\cmake" -DOPENSSL_ROOT_DIR="%CD%\..\openssl-1.1\x64" -DTWITCH_CLIENTID="%TWITCH-CLIENTID%" -DTWITCH_HASH="%TWITCH-HASH%"  -DRESTREAM_CLIENTID="%RESTREAM-CLIENTID%" -DRESTREAM_HASH="%RESTREAM-HASH%" ..
)
if "%1"=="Millicast" (
  move obs-studio.sln Millicast.sln
)
cd ..
