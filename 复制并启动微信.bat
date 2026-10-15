@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo ========================================================
echo     WeChat-Hook 4.1.15.13 一键编译、部署与启动
echo ========================================================
echo.

echo [1/4] 正在检测 MSBuild 编译工具...
set "MSBUILD="

where msbuild >nul 2>&1
if %errorlevel% equ 0 set "MSBUILD=msbuild"

if not defined MSBUILD (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
        for /f "delims=" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe') do (
            if exist "%%i" set "MSBUILD=%%i"
        )
    )
)

if not defined MSBUILD (
    for %%p in (
        "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
        "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
        "%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
        "D:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
        "D:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
        "D:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    ) do (
        if not defined MSBUILD if exist %%p set "MSBUILD=%%~p"
    )
)

if not defined MSBUILD (
    echo [错误] 未能找到 MSBuild.exe！
    echo 请确保已安装 Visual Studio 或将 MSBuild 加入系统 PATH 环境变量。
    pause
    exit /b 1
)

echo 已找到 MSBuild: "%MSBUILD%"
echo.

echo [2/4] 正在编译 version.dll (Release x64)...
"%MSBUILD%" "%~dp0x64_Version_dll.sln" /p:Configuration=Release /p:Platform=x64 /t:Build /m /v:m
if %errorlevel% neq 0 (
    echo.
    echo [错误] 项目编译失败！请检查上述编译报错。
    pause
    exit /b 1
)
echo 编译成功！
echo.

echo [3/4] 正在终止运行中的微信进程...
taskkill /F /IM Weixin.exe >nul 2>&1
taskkill /F /IM WeChat.exe >nul 2>&1
ping -n 2 127.0.0.1 >nul
echo 微信进程已清理。
echo.

set "WEIXIN_DIR=C:\Program Files\Tencent\Weixin"
if not exist "%WEIXIN_DIR%\Weixin.exe" (
    for /f "tokens=2*" %%a in ('reg query "HKCU\Software\Tencent\Weixin" /v InstallPath 2^>nul ^| findstr "InstallPath"') do (
        set "WEIXIN_DIR=%%b"
    )
)

if not exist "%WEIXIN_DIR%\Weixin.exe" (
    echo [错误] 未找到微信安装目录: "%WEIXIN_DIR%"
    pause
    exit /b 1
)

echo [4/4] 正在部署最新编译的 version.dll 到微信目录...
copy /Y "%~dp0x64\Release\version.dll" "%WEIXIN_DIR%\version.dll" >nul 2>&1
if %errorlevel% equ 0 goto :copy_ok

echo [权限提示] 写入微信目录需要管理员权限，正在请求提权覆盖...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$p = Start-Process cmd.exe -ArgumentList '/c copy /Y \"\"%~dp0x64\Release\version.dll\"\" \"\"%WEIXIN_DIR%\version.dll\"\"' -Verb RunAs -PassThru -Wait; exit $p.ExitCode" 2>nul
if %errorlevel% neq 0 (
    echo.
    echo [错误] 复制 version.dll 到微信目录失败！
    echo 请右键选择【以管理员身份运行】此批处理文件。
    pause
    exit /b 1
)

:copy_ok
echo 部署成功！正在启动微信 4.1.15.13...
start "" "%WEIXIN_DIR%\Weixin.exe"
echo.
echo 微信已启动！Hook 服务将在微信主窗口加载完成后在端口 30001 就绪。
ping -n 4 127.0.0.1 >nul
exit /b 0
