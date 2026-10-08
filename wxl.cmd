@echo off
rem Windows entry point. macOS/Linux: use ./wxl
where py >nul 2>nul
if %ERRORLEVEL%==0 (
    py -3 "%~dp0tools\wxl.py" %*
) else (
    python "%~dp0tools\wxl.py" %*
)
