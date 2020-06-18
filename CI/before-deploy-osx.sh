hr() {
  echo "───────────────────────────────────────────────────"
  echo $1
  echo "───────────────────────────────────────────────────"
}

# Exit if something fails
set -e

# Generate file name variables
export APP_NAME=OBS
export LIBWEBRTC_REV=79
export DEPLOY_VERSION=23.2
export GIT_HASH=$(git rev-parse --short HEAD)
export FILE_DATE=$(date +%Y-%m-%d.%H:%M:%S)
export FILENAME=$APP_NAME-m$LIBWEBRTC_REVv$DEPLOY_VERSION-$FILE_DATE-$GIT_HASH-osx.pkg
# NOTE ALEX: from command line or ENV please
export BUILD_CONFIG=Release

# NOTE ALEX: fix me
# cd ./build

# Package everything into a nice .app
hr "Packaging .app"
../CI/install/osx/packageApp.sh

# fix obs outputs
cp /usr/local/opt/mbedtls/lib/libmbedtls.12.dylib ./$APP_NAME.app/Contents/Frameworks/
cp /usr/local/opt/mbedtls/lib/libmbedcrypto.3.dylib ./$APP_NAME.app/Contents/Frameworks/
cp /usr/local/opt/mbedtls/lib/libmbedx509.0.dylib ./$APP_NAME.app/Contents/Frameworks/
install_name_tool -change /usr/local/opt/mbedtls/lib/libmbedtls.12.dylib     @executable_path/../Frameworks/libmbedtls.12.dylib   ./$APP_NAME.app/Contents/Plugins/obs-outputs.so
install_name_tool -change /usr/local/opt/mbedtls/lib/libmbedcrypto.3.dylib   @executable_path/../Frameworks/libmbedcrypto.3.dylib ./$APP_NAME.app/Contents/Plugins/obs-outputs.so
install_name_tool -change /usr/local/opt/mbedtls/lib/libmbedx509.0.dylib     @executable_path/../Frameworks/libmbedx509.0.dylib   ./$APP_NAME.app/Contents/Plugins/obs-outputs.so
install_name_tool -change /usr/local/opt/curl/lib/libcurl.4.dylib            @executable_path/../Frameworks/libcurl.4.dylib       ./$APP_NAME.app/Contents/Plugins/obs-outputs.so
install_name_tool -change @rpath/libobs.0.dylib                              @executable_path/../Frameworks/libobs.0.dylib        ./$APP_NAME.app/Contents/Plugins/obs-outputs.so

# NOTE ALEX: specific to cosmo websocket plugin - should consolidate with other openssl/mbedtls at one point
install_name_tool -change /usr/local/opt/openssl@1.1/lib/libssl.1.1.dylib    @executable_path/../Frameworks/libssl.1.1.dylib      ./$APP_NAME.app/Contents/Plugins/obs-outputs.so
install_name_tool -change /usr/local/opt/openssl@1.1/lib/libcrypto.1.1.dylib @executable_path/../Frameworks/libcrypto.1.1.dylib   ./$APP_NAME.app/Contents/Plugins/obs-outputs.so

# NOTE ALEX: no update 
# copy sparkle into the app
# hr "Copying Sparkle.framework"
# cp -r ../../sparkle/Sparkle.framework ./$APP_NAME.app/Contents/Frameworks/
# install_name_tool -change @rpath/Sparkle.framework/Versions/A/Sparkle @executable_path/../Frameworks/Sparkle.framework/Versions/A/Sparkle ./$APP_NAME.app/Contents/MacOS/ebs

# NOTE ALEX: enable CEF LATER
# Copy Chromium embedded framework to app Frameworks directory
# hr "Copying Chromium Embedded Framework.framework"
# sudo mkdir -p $APP_NAME.app/Contents/Frameworks
# sudo cp -r ../../cef_binary_${CEF_BUILD_VERSION}_macosx64/Release/Chromium\ Embedded\ Framework.framework $APP_NAME.app/Contents/Frameworks/

# install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui         @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui         ./$APP_NAME.app/Contents/Plugins/obs-browser.so
# install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore       @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore       ./$APP_NAME.app/Contents/Plugins/obs-browser.so
# install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./$APP_NAME.app/Contents/Plugins/obs-browser.so

# edit plist
plutil -insert CFBundleVersion            -string $DEPLOY_VERSION ./$APP_NAME.app/Contents/Info.plist
plutil -insert CFBundleShortVersionString -string $DEPLOY_VERSION ./$APP_NAME.app/Contents/Info.plist

# plutil -insert $APP_NAMEFeedsURL        -string https://obsproject.com/osx_update/feeds.xml          ./$APP_NAME.app/Contents/Info.plist
# plutil -insert SUFeedURL                -string https://obsproject.com/osx_update/stable/updates.xml ./$APP_NAME.app/Contents/Info.plist

# This is only needed for Sparkle Update framework
# plutil -insert SUPublicDSAKeyFile -string $APP_NAMEPublicDSAKey.pem ./$APP_NAME.app/Contents/Info.plist
# cp ../CI/install/osx/$APP_NAMEPublicDSAKey.pem $APP_NAME.app/Contents/Resources

# NOTE ALEX: MacOS Catalina might make problem about python
# had to use easy_install pip / pip install dmgbuild / and then change the path to add python-bin
# anyway, the app needs to be signed before we make the package, and then the package needs to be signed too
dmgbuild -s ../CI/install/osx/settings.json "$APP_NAME" rfs.dmg

