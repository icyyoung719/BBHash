@echo off
REM Windows 构建脚本，使用CMake或直接使用MSVC编译器

echo ===== BBHash Windows 构建脚本 =====

REM 检查是否有CMake
where cmake >nul 2>nul
if %ERRORLEVEL% == 0 (
    echo 使用CMake构建...
    if not exist "build" mkdir build
    cd build
    cmake .. -G "Visual Studio 16 2019" -A x64
    cmake --build . --config Release
    cd ..
    echo CMake构建完成!
    exit /b 0
)

REM 如果没有CMake，使用直接编译方式
echo 未找到CMake，使用直接编译方式...

REM 检查cl.exe是否在PATH中
where cl >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo 未找到Visual C++编译器(cl.exe)。
    echo 请运行VS的开发者命令提示符，或将Visual C++添加到PATH中。
    exit /b 1
)

echo 使用MSVC编译器直接编译...

REM 编译示例
cl.exe /EHsc /O2 /std:c++14 /D_CRT_SECURE_NO_WARNINGS /W3 example.cpp /Fe:example.exe

REM 编译自定义哈希示例
cl.exe /EHsc /O2 /std:c++14 /D_CRT_SECURE_NO_WARNINGS /W3 example_custom_hash.cpp /Fe:example_custom_hash.exe

REM 编译测试程序
cl.exe /EHsc /O2 /std:c++14 /D_CRT_SECURE_NO_WARNINGS /W3 bootest.cpp /Fe:Bootest.exe

echo 编译完成!
