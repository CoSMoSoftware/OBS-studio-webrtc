if exist Qt_5.15.2.7z (curl -u %FTP_LOGIN%:%FTP_PASSWORD% -kLO %FTP_PATH_PREFIX%/Qt_5.15.2.7z -f --retry 5 -z Qt_5.15.2.7z) else (curl -u %FTP_LOGIN%:%FTP_PASSWORD% -kLO %FTP_PATH_PREFIX%/Qt_5.15.2.7z -f --retry 5 -C -)
7z x Qt_5.15.2.7z -oQt
mv Qt C:\QtDep
dir C:\QtDep
