@echo off
rem Native DOS bootstrap of acc.exe (the Makefile builds it with acc).
rem Keep the module list in step with COMMON and DOS_OBJECTS in Makefile.
if "%1" == "" goto usage
cc +0 +c -b -n -DACC_DOS -o acc.o acc.c
if errorlevel 1 goto failed
cc +0 +c -b -n -DACC_DOS -o words.o words.c
if errorlevel 1 goto failed
cc +0 +c -b -n -DACC_DOS -o args.o args.c
if errorlevel 1 goto failed
cc +0 +c -b -n -DACC_DOS -o profile.o profile.c
if errorlevel 1 goto failed
cc +0 +c -b -n -DACC_DOS -o raw.o raw.c
if errorlevel 1 goto failed
cc +0 +c -b -n -DACC_DOS -o path.o path.c
if errorlevel 1 goto failed
cc +0 +c -b -n -DACC_DOS -o plan.o plan.c
if errorlevel 1 goto failed
cc +0 +c -b -n -DACC_DOS -o print.o print.c
if errorlevel 1 goto failed
cc +0 +c -b -n -DACC_DOS -o cmdhdr.o cmdhdr.c
if errorlevel 1 goto failed
cc +0 +c -b -n -DACC_DOS -o execdos.o execdos.c
if errorlevel 1 goto failed
lb %1\lib\c.lib -x sbegin
if errorlevel 1 goto failed
ln -o acc.exe sbegin.o acc.o words.o args.o profile.o raw.o path.o plan.o print.o cmdhdr.o execdos.o -lc
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
