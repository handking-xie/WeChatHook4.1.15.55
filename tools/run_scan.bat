call "D:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
cl /nologo /O2 /EHsc /Fetools\scan_qr.exe tools\scan_qr.cpp
tools\scan_qr.exe %1
