if exist Qt_5.10.1.7z (curl -kLO https://cdn-fastly.obsproject.com/downloads/Qt_5.10.1.7z -f --retry 5 -z Qt_5.10.1.7z) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/Qt_5.10.1.7z -f --retry 5 -C -)
7z x Qt_5.10.1.7z -oQt
mv Qt C:\QtDep
set QTDIR64=C:\QtDep\5.10.1\msvc2017_64
if exist dependencies2017.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/dependencies2017.zip -f --retry 5 -z dependencies2017.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/dependencies2017.zip -f --retry 5 -C -)
7z x dependencies2017.zip -odependencies2017
set DepsPath64=%CD%\dependencies2017\win64
if exist vlc.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/vlc.zip -f --retry 5 -z vlc.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/vlc.zip -f --retry 5 -C -)
7z x vlc.zip -ovlc
set VLCPath=%CD%\vlc
if exist cef_binary_%CEF_VERSION%_windows64.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows64.zip -f --retry 5 -z cef_binary_%CEF_VERSION%_windows64.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows64.zip -f --retry 5 -C -)
7z x cef_binary_%CEF_VERSION%_windows64.zip -oCEF_64
set CEF_64=%CD%\CEF_64\cef_binary_%CEF_VERSION%_windows64
curl -c ./cookie -s -L "https://drive.google.com/uc?export=download&id=1mgOr53httBCxmmIoln4VozIdrw-sOUuY" > nul
type cookie
for /f "delims=" %%x in ('findstr /C:"NID" cookie') do set "confirm_line=%%x"
echo %confirm_line%
set confirm_id=%confirm_line:*186\==%
echo %confirm_id%

rem curl -Lb ./cookie "https://drive.google.com/uc?export=download&confirm=`awk '/download/ {print $NF}' ./cookie`&id=1mgOr53httBCxmmIoln4VozIdrw-sOUuY" -olibWebRTC-73.0-x64-Rel-msvc2017-COMMUNITY-BETA.zip
rem 7z x libWebRTC-73.0-x64-Rel-msvc2017-COMMUNITY-BETA.zip -olibWebRTC-73
rem curl -c ./cookie -s -L "https://drive.google.com/uc?export=download&id=1nwuNAq2N9egnVGCmZ-_3JlUCI6-EroSL" > nul
rem curl -Lb ./cookie "https://drive.google.com/uc?export=download&confirm=`awk '/download/ {print $NF}' ./cookie`&id=1nwuNAq2N9egnVGCmZ-_3JlUCI6-EroSL" -oopenssl-1.1.zip
rem 7z x openssl-1.1.zip -oopenssl-1.1
set build_config=Release
mkdir build64
cd build64
cmake -G "Visual Studio 15 2017 Win64" -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_64% -Dlibwebrtc_DIR=%CD%\libwebrtc-73\cmake -DOPENSSL_ROOT_DIR=%CD%\openssl-1.1\openssl-1.1\x64  ..
cd ..