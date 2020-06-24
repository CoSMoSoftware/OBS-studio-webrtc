hdiutil create /tmp/tmp.dmg -ov -volname "EBS_m73v23.2_Install" -fs HFS+ -srcfolder "./EBS.app" 
hdiutil convert /tmp/tmp.dmg -format UDZO -o ./EBS_m73v23.2_Install.dmg
