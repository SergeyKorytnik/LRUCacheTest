@echo off
rem The script builds both x86 and x64 Release versions of
rem LRUCacheTest.exe

rem This is the default location for MSBuild application.
set MSBUILD="%ProgramFiles(x86)%\MSBuild\14.0\bin\amd64\MSBuild.exe"

echo Building 32-bit version of LRUCacheTest.exe ...
%MSBUILD% /nologo /p:Configuration=Release /p:Platform=x86 /m:4 /t:build /v:m LRUCacheTest.sln
if errorlevel 1 exit

echo Building 64-bit version of LRUCacheTest.exe ...
%MSBUILD% /nologo /p:Configuration=Release /p:Platform=x64 /m:4 /t:build /v:m LRUCacheTest.sln
if errorlevel 1 exit
