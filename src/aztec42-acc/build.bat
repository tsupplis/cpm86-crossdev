@echo off
if "%1" == "" goto usage
cc +0 -b -n -o acc.o acc.c
if errorlevel 1 goto failed
lb %1\lib\c.lib -x sbegin
if errorlevel 1 goto failed
ln -o acc.exe sbegin.o acc.o -lc
if errorlevel 1 goto failed
echo ACC BUILD PASS
goto end
:usage
echo Usage: build C:\AZTEC
echo Set PATH, INCLUDE and CLIB for that Aztec installation first.
goto end
:failed
echo ACC BUILD FAILED
:end
