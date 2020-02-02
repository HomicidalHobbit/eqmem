@echo off
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.24.28314\bin\Hostx64\x64\cl.exe" /c /EHsc /I.. /I"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.24.28314\include" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.18362.0\ucrt" ..\eqmem.cpp ..\memmanager.cpp ..\memtracker.cpp ..\bin.cpp ..\bucket.cpp
libman eqmem.obj memmanager.obj memtracker.obj bin.obj bucket.obj -o eqmem.lib
md ..\..\lib\win64
copy eqmem.lib ..\..\lib\win64
erase *.obj
erase eqmem.lib


