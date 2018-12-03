@echo OFF

rem set protocpath=D:\src\vcpkg\installed\x86-windows\tools\
set protocpath=D:\bin\

%protocpath%protoc.exe -I . --cpp_out=..\logon logon.proto -o logon.pb

echo "logon.proto generated."

PAUSE