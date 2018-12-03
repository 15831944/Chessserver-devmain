@echo OFF
setlocal enabledelayedexpansion enableextensions

set protocpath=D:\src\vcpkg\installed\x86-windows\tools\

set LIST=
for %%x in (*.proto) do set LIST=!LIST! %%x
set LIST=%LIST:~1%

%protocpath%protoc.exe -I . --grpc_out=..\ --cpp_out=..\ --plugin=protoc-gen-grpc=%protocpath%grpc\grpc_cpp_plugin.exe %LIST%

echo "cpp file generated."

rem !Obsolete! go shuld seperate genteration while there are multiple packages
rem %protocpath%protoc.exe -I . --go_out=..\ --plugin=grpc qpserver.proto
rem echo "go file generated."

PAUSE