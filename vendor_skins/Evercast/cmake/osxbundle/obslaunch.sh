#!/bin/sh
# Copyright Dr. Alex. Gouaillard (2015, 2020)

# use argument 1 as the version or get it from sw_vers
os_ver=$(sw_vers -productVersion)

if [[ "$os_ver" == 10.15.* ]]; then
	echo "macOS Catalina"
	osascript -e "tell app \"Terminal\" to do script \"cd /Applications/EBS.app/Contents/Resources/bin && ./ebs\""
else
	cd "$(dirname "$0")"
  cd ../Resources/bin
  exec ./ebs "$@"
fi
