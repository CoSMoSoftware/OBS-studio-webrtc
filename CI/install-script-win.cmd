rem ---------------------------------------------------------------------
if exist Qt_5.10.1.7z (curl -kLO https://cdn-fastly.obsproject.com/downloads/Qt_5.10.1.7z -f --retry 5 -z Qt_5.10.1.7z) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/Qt_5.10.1.7z -f --retry 5 -C -)
7z x Qt_5.10.1.7z -oQt
mv Qt C:\QtDep
set QTDIR64=C:\QtDep\5.10.1\msvc2017_64
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
curl -c ./cookie -s -L "https://drive.google.com/uc?export=download&id=1mgOr53httBCxmmIoln4VozIdrw-sOUuY" > nul
for /f "delims="  %%x in ('findstr /C:"NID" cookie') do set "confirm_line=%%x"
for /f "tokens=2 delims==" %%a in ("%confirm_line%") do set "confirm_id=%%a"
curl -Lb ./cookie "https://drive.google.com/uc?export=download&confirm=%confirm_id%&id=1mgOr53httBCxmmIoln4VozIdrw-sOUuY" -olibWebRTC-73.0-x64-Rel-msvc2017-COMMUNITY-BETA.zip
7z x libWebRTC-73.0-x64-Rel-msvc2017-COMMUNITY-BETA.zip -olibWebRTC-73
set libwebrtcPath=%CD%\libWebRTC-73\cmake
rem ---------------------------------------------------------------------
curl -c ./cookie -s -L "https://drive.google.com/uc?export=download&id=1nwuNAq2N9egnVGCmZ-_3JlUCI6-EroSL" > nul
for /f "delims=" %%x in ('findstr /C:"NID" cookie') do set "confirm_line=%%x"
for /f "tokens=2 delims==" %%a in ("%confirm_line%") do set "confirm_id=%%a"
curl -Lb ./cookie "https://drive.google.com/uc?export=download&confirm=%confirm_id%&id=1nwuNAq2N9egnVGCmZ-_3JlUCI6-EroSL" -oopenssl-1.1.zip
7z x openssl-1.1.zip -oopenssl-1.1
set opensslPath=%CD%\openssl-1.1\openssl-1.1\x64
