@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo ========================================================
echo     WeChat-Hook 4.1.15.13 一键编译、部署与启动
echo ========================================================
echo.

:: 1. 检测管理员权限，未提升则请求以管理员身份运行
net session >nul 2>&1
if %errorlevel% neq 0 goto :elevate

goto :main

:elevate
echo [权限提示] 部署到 Program Files 目录需要管理员权限。
echo 正在请求提升为管理员权限运行...
powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process cmd.exe -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs" 2>nul
if %errorlevel% neq 0 (
    echo.
    echo [错误] 提升管理员权限失败。
    echo 请右键选择【以管理员身份运行】此批处理文件！
    echo.
    pause
)
exit /b

:main
:: 2. 查找 MSBuild 编译工具
