@echo off

cargo test
if errorlevel 1 exit /b 1

cargo clippy
if errorlevel 1 exit /b 1

@REM cargo build
@REM cargo build --target x86_64-pc-windows-msvc
cargo build --target i686-pc-windows-msvc

@REM cargo build --release
@REM cargo build --release --target x86_64-pc-windows-msvc
cargo build --release --target i686-pc-windows-msvc

mkdir ..\Debug             2>nul
mkdir ..\Release           2>nul
mkdir ..\ReleaseWithDebug  2>nul

copy target\i686-pc-windows-msvc\debug\rustlib.dll   ..\Debug
copy target\i686-pc-windows-msvc\release\rustlib.dll ..\Release
copy target\i686-pc-windows-msvc\release\rustlib.dll ..\ReleaseWithDebug
