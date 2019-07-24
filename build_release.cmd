@echo off

rmdir build /S/Q
mkdir build

xcopy /Y/E config build\config\
mkdir build\logs\
copy /y NUL build\logs\cbot.log >NUL
copy /y NUL build\logs\cbot_errors.log >NUL

xcopy /Y/E scripts build\scripts\
xcopy /Y/E cbt build\cbt\
xcopy src\Release\cbot.exe build\
xcopy src\DroidSansMono.ttf build\
xcopy src\cbot.config build\
xcopy src\cbot_default.cmd build\
xcopy src\README.txt build\

rmdir release /S/Q
mkdir release

:: Contingent on having 7zip installed here
cd build
del ..\release\cbot.zip
C:\7z\7z.exe a -tzip ..\release\cbot.zip *
cd ..

:: Clean up build
rmdir build /S/Q
pause