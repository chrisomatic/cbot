:: CBOT BUILD
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\amd64\vcvars64.bat"

cl /OUT:"C:\dev\C\cbot\Debug\cbot.exe" /LINK "kernel32.lib" "user32.lib" "gdi32.lib" "shell32.lib" /MACHINE:X86
