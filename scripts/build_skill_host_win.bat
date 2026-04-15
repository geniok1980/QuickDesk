@echo off

echo=
echo=
echo ---------------------------------------------------------------
echo build quickdesk-skill-host + built-in skills (Rust workspace)
echo ---------------------------------------------------------------

:: get script absolute path
set script_path=%~dp0
set old_cd=%cd%
cd /d %~dp0

SETLOCAL EnableDelayedExpansion
set build_mode=release
set errno=1

echo=
echo=
echo ---------------------------------------------------------------
echo parse arguments
echo ---------------------------------------------------------------

:parse_args
if "%1"=="" goto args_done
if /i "%1"=="debug" set build_mode=debug
if /i "%1"=="release" set build_mode=release
shift
goto parse_args
:args_done

echo [*] build mode: %build_mode%
echo=

set skill_host_dir=%script_path%..\quickdesk-skill-host
set output_path=%script_path%..\output\x64

echo [*] skill-host workspace dir: %skill_host_dir%
echo [*] output path: %output_path%

:: check if Rust is installed
where cargo >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] error: cargo not found. Please install Rust: https://rustup.rs
    goto return
)

:: build
cd /d "%skill_host_dir%"
echo [*] building quickdesk-skill-host workspace...

if /i "%build_mode%"=="debug" (
    cargo build
    if not %errorlevel%==0 (
        echo [!] cargo build failed
        goto return
    )
    set cargo_out=%skill_host_dir%\target\debug
    set dest_dir=%output_path%\Debug
) else (
    cargo build --release
    if not %errorlevel%==0 (
        echo [!] cargo build failed
        goto return
    )
    set cargo_out=%skill_host_dir%\target\release
    set dest_dir=%output_path%\Release
)

:: copy skill-host binary to output directory
if not exist "!dest_dir!" mkdir "!dest_dir!"
copy /Y "!cargo_out!\quickdesk-skill-host.exe" "!dest_dir!\" >nul
echo [*] copied quickdesk-skill-host.exe to !dest_dir!

:: copy skill binaries and SKILL.md into per-skill subdirectories
set skills_src=%skill_host_dir%\skills
for %%s in (sys-info file-ops shell-runner python-executor) do (
    if not exist "!dest_dir!\skills\%%s" mkdir "!dest_dir!\skills\%%s"
    copy /Y "!cargo_out!\%%s.exe" "!dest_dir!\skills\%%s\" >nul
    copy /Y "!skills_src!\%%s\SKILL.md" "!dest_dir!\skills\%%s\" >nul
    echo [*] copied %%s/%%s.exe + SKILL.md
)

echo=
echo=
echo ---------------------------------------------------------------
echo [*] quickdesk-skill-host build finished!
echo ---------------------------------------------------------------

set errno=0

:return
cd /d "%old_cd%"
exit /B %errno%

ENDLOCAL
