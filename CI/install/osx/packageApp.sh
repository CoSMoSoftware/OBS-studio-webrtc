QT_REV=5.10.1
EXE_NAME=obs

# Exit if something fails
set -e

rm -rf ./$APP_NAME.app

mkdir $APP_NAME.app
mkdir $APP_NAME.app/Contents
mkdir $APP_NAME.app/Contents/MacOS
mkdir $APP_NAME.app/Contents/PlugIns
mkdir $APP_NAME.app/Contents/Resources

cp -r rundir/$BUILD_CONFIG/bin/         ./$APP_NAME.app/Contents/MacOS
cp -r rundir/$BUILD_CONFIG/data         ./$APP_NAME.app/Contents/Resources
cp ../CI/install/osx/obs.icns           ./$APP_NAME.app/Contents/Resources
cp -r rundir/$BUILD_CONFIG/obs-plugins/ ./$APP_NAME.app/Contents/PlugIns
cp ../CI/install/osx/Info.plist         ./$APP_NAME.app/Contents

../CI/install/osx/dylibBundler -b -cd -d ./$APP_NAME.app/Contents/Frameworks -p @executable_path/../Frameworks/ \
-s ./$APP_NAME.app/Contents/MacOS \
-s /usr/local/opt/mbedtls/lib/ \
-x ./$APP_NAME.app/Contents/PlugIns/coreaudio-encoder.so \
-x ./$APP_NAME.app/Contents/PlugIns/decklink-ouput-ui.so \
-x ./$APP_NAME.app/Contents/PlugIns/frontend-tools.so \
-x ./$APP_NAME.app/Contents/PlugIns/image-source.so \
-x ./$APP_NAME.app/Contents/PlugIns/linux-jack.so \
-x ./$APP_NAME.app/Contents/PlugIns/mac-avcapture.so \
-x ./$APP_NAME.app/Contents/PlugIns/mac-capture.so \
-x ./$APP_NAME.app/Contents/PlugIns/mac-decklink.so \
-x ./$APP_NAME.app/Contents/PlugIns/mac-syphon.so \
-x ./$APP_NAME.app/Contents/PlugIns/mac-vth264.so \
-x ./$APP_NAME.app/Contents/PlugIns/obs-browser.so \
-x ./$APP_NAME.app/Contents/PlugIns/obs-browser-page \
-x ./$APP_NAME.app/Contents/PlugIns/obs-ffmpeg.so \
-x ./$APP_NAME.app/Contents/PlugIns/obs-filters.so \
-x ./$APP_NAME.app/Contents/PlugIns/obs-transitions.so \
-x ./$APP_NAME.app/Contents/PlugIns/obs-vst.so \
-x ./$APP_NAME.app/Contents/PlugIns/rtmp-services.so \
-x ./$APP_NAME.app/Contents/MacOS/$EXE_NAME \
-x ./$APP_NAME.app/Contents/MacOS/obs-ffmpeg-mux \
-x ./$APP_NAME.app/Contents/MacOS/obslua.so \
-x ./$APP_NAME.app/Contents/PlugIns/obs-x264.so \
-x ./$APP_NAME.app/Contents/PlugIns/text-freetype2.so \
-x ./$APP_NAME.app/Contents/PlugIns/obs-libfdk.so
# -x ./$APP_NAME.app/Contents/MacOS/_obspython.so \
# -x ./$APP_NAME.app/Contents/PlugIns/obs-outputs.so \

/usr/local/Cellar/qt/5.14.1/bin/macdeployqt ./$APP_NAME.app

mv ./$APP_NAME.app/Contents/MacOS/libobs-opengl.so ./$APP_NAME.app/Contents/Frameworks

rm -f -r ./OBS.app/Contents/Frameworks/QtNetwork.framework

# put qt network in here becasuse streamdeck uses it
cp -R /usr/local/opt/qt/lib/QtNetwork.framework ./OBS.app/Contents/Frameworks
chmod -R +w ./OBS.app/Contents/Frameworks/QtNetwork.framework
rm -r ./OBS.app/Contents/Frameworks/QtNetwork.framework/Headers
rm -r ./OBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/Headers/
chmod 644 ./OBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/Resources/Info.plist
install_name_tool -id @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork ./OBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork
install_name_tool -change /usr/local/Cellar/qt/5.14.1/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./OBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork


# decklink ui qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./$APP_NAME.app/Contents/PlugIns/decklink-ouput-ui.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./$APP_NAME.app/Contents/PlugIns/decklink-ouput-ui.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./$APP_NAME.app/Contents/PlugIns/decklink-ouput-ui.so

# frontend tools qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./$APP_NAME.app/Contents/PlugIns/frontend-tools.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./$APP_NAME.app/Contents/PlugIns/frontend-tools.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./$APP_NAME.app/Contents/PlugIns/frontend-tools.so

# vst qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./$APP_NAME.app/Contents/PlugIns/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./$APP_NAME.app/Contents/PlugIns/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./$APP_NAME.app/Contents/PlugIns/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtMacExtras.framework/Versions/5/QtMacExtras @executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras ./$APP_NAME.app/Contents/PlugIns/obs-vst.so
