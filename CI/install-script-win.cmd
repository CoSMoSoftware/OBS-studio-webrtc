rem ---------------------------------------------------------------------
if exist Qt_5.10.1.7z (curl -kLO https://cdn-fastly.obsproject.com/downloads/Qt_5.10.1.7z -f --retry 5 -z Qt_5.10.1.7z) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/Qt_5.10.1.7z -f --retry 5 -C -)
7z x Qt_5.10.1.7z -oQt
set QTDIR64=%CD%\Qt\5.10.1\msvc2017_64
rem ---------------------------------------------------------------------
if exist dependencies2017.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/dependencies2017.zip -f --retry 5 -z dependencies2017.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/dependencies2017.zip -f --retry 5 -C -)
7z x dependencies2017.zip -odependencies2017
set DepsPath64=%CD%\dependencies2017\win64
rem ---------------------------------------------------------------------
if exist vlc.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/vlc.zip -f --retry 5 -z vlc.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/vlc.zip -f --retry 5 -C -)
7z x vlc.zip -ovlc
set VLCPath=%CD%\vlc
rem ---------------------------------------------------------------------
rem if exist cef_binary_%CEF_VERSION%_windows64.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows64.zip -f --retry 5 -z cef_binary_%CEF_VERSION%_windows64.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows64.zip -f --retry 5 -C -)
rem 7z x cef_binary_%CEF_VERSION%_windows64.zip -oCEF_64
rem set CEF_64=%CD%\CEF_64\cef_binary_%CEF_VERSION%_windows64
rem ---------------------------------------------------------------------
if exist libWebRTC-73-win.tar.gz (curl -kLO https://libwebrtc-community-builds.s3.amazonaws.com/libWebRTC-73-win.tar.gz -f --retry 5 -z libWebRTC-73-win.tar.gz) else (curl -kLO https://libwebrtc-community-builds.s3.amazonaws.com/libWebRTC-73-win.tar.gz -f --retry 5 -C -)
tar -xzf libWebRTC-73-win.tar.gz
set libwebrtcPath=%CD%\libWebRTC-73\cmake
rem ---------------------------------------------------------------------
if exist openssl-1.1.tgz (curl -kLO https://libwebrtc-community-builds.s3.amazonaws.com/openssl-1.1.tgz -f --retry 5 -z openssl-1.1.tgz) else                 (curl -kLO https://libwebrtc-community-builds.s3.amazonaws.com/openssl-1.1.tgz -f --retry 5 -C -)
tar -xzf openssl-1.1.tgz
set opensslPath=%CD%\openssl-1.1\openssl-1.1\x64
rem ---------------------------------------------------------------------
set build_config=Release
mkdir build64
cd build64
