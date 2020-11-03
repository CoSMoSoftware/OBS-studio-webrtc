if exist dependencies2017.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/dependencies2017.zip -f --retry 5 -z dependencies2017.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/dependencies2017.zip -f --retry 5 -C -)
if exist vlc.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/vlc.zip -f --retry 5 -z vlc.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/vlc.zip -f --retry 5 -C -)
if exist cef_binary_%CEF_VERSION%_windows64_minimal.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows64_minimal.zip -f --retry 5 -z cef_binary_%CEF_VERSION%_windows64_minimal.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows64_minimal.zip -f --retry 5 -C -)
if exist libWebRTC-%LIBWEBRTC_VERSION%-x64-RelComm.exe (curl -kLO https://www.palakis.fr/obs/obs-studio-webrtc/libWebRTC-%LIBWEBRTC_VERSION%-x64-RelComm.exe -f --retry 5 -z libWebRTC-%LIBWEBRTC_VERSION%-x64-RelComm.exe) else (curl -kLO https://www.palakis.fr/obs/obs-studio-webrtc/libWebRTC-%LIBWEBRTC_VERSION%-x64-RelComm.exe -f --retry 5 -C -)
if exist openssl-1.1.tgz (curl -kLO https://libwebrtc-community-builds.s3.amazonaws.com/openssl-1.1.tgz -f --retry 5 -z openssl-1.1.tgz) else                 (curl -kLO https://libwebrtc-community-builds.s3.amazonaws.com/openssl-1.1.tgz -f --retry 5 -C -)
7z x dependencies2017.zip -odependencies2017
7z x vlc.zip -ovlc
7z x cef_binary_%CEF_VERSION%_windows64_minimal.zip -oCEF_64
"libWebRTC-%LIBWEBRTC_VERSION%-x64-RelComm.exe" /S /SD /D="%CD%\libwebrtc"
tar -xzf openssl-1.1.tgz
set DepsPath64=%CD%\dependencies2017\win64
set VLCPath=%CD%\vlc
set QTDIR64=C:\QtDep\5.10.1\msvc2017_64
set CEF_64=%CD%\CEF_64\cef_binary_%CEF_VERSION%_windows64_minimal
set libwebrtcPath=%CD%\libwebrtc\cmake
set opensslPath=%CD%\openssl-1.1\x64
mkdir build build32 build64
if "%TWITCH-CLIENTID%"=="$(twitch_clientid)" (
cd build64
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_SYSTEM_VERSION=10.0 -DOBS_VERSION_OVERRIDE="26.0.2" -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DENABLE_VLC=ON -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_64% -DWEBRTC_ROOT_DIR="%CD%\libwebrtc" -Dlibwebrtc_DIR="%CD%\libwebrtc\cmake" -DOPENSSL_ROOT_DIR="%CD%\openssl-1.1\x64" ..
) else (
cd build64
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_SYSTEM_VERSION=10.0 -DOBS_VERSION_OVERRIDE="26.0.2" -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DENABLE_VLC=ON -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_64% -DWEBRTC_ROOT_DIR="%CD%\libwebrtc" -Dlibwebrtc_DIR="%CD%\libwebrtc\cmake" -DOPENSSL_ROOT_DIR="%CD%\openssl-1.1\x64" -DTWITCH_CLIENTID="%TWITCH-CLIENTID%" -DTWITCH_HASH="%TWITCH-HASH%"  -DRESTREAM_CLIENTID="%RESTREAM-CLIENTID%" -DRESTREAM_HASH="%RESTREAM-HASH%" ..
)
cd ..
