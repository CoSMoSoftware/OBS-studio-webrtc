robocopy C:\projects\ebs-studio\build32\rundir\RelWithDebInfo C:\projects\ebs-studio\build\ /E /XF .gitignore
robocopy C:\projects\ebs-studio\build64\rundir\RelWithDebInfo C:\projects\ebs-studio\build\ /E /XC /XN /XO /XF .gitignore
7z a build.zip C:\projects\ebs-studio\build\*
