% Copyright Dr. Alex. Gouaillard (2015, 2020)

robocopy C:\projects\ebs-studio\build32\rundir\%build_config% C:\projects\ebs-studio\build\ /E /XF .gitignore
robocopy C:\projects\ebs-studio\build64\rundir\%build_config% C:\projects\ebs-studio\build\ /E /XC /XN /XO /XF .gitignore
7z a build.zip C:\projects\ebs-studio\build\*
