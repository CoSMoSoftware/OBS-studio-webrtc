robocopy .\build32\rundir\Release .\build\ /E /XF .gitignore
robocopy .\build64\rundir\Release .\build\ /E /XC /XN /XO /XF .gitignore
7z a build.zip .\build\*
