@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.

IF "%1" == "release" (
    echo Generating RELEASE build...
    set OUT_DIR=Release
    set OPT_FLAG=/O2
    ) ELSE (
    echo Generating DEBUG build...
    set OUT_DIR=Debug
    set OPT_FLAG=/Zi
    )

set OUT_EXE=chip8.exe
set SDL2_DIR=E:\Projects\SDL\SDL2-devel-2.0.9-VC\SDL2-2.0.9
set INCLUDES=/I.. /I..\.. /I%SDL2_DIR%\include
set SOURCES=main.cpp Chip8.cpp Chip8Sound.cpp CTexture.cpp .\imgui\imgui_impl_sdl.cpp .\imgui\imgui_impl_opengl2.cpp .\imgui\imgui*.cpp
set LIBS=/libpath:%SDL2_DIR%\lib\x64 SDL2.lib SDL2main.lib opengl32.lib
mkdir %OUT_DIR%
cl /EHsc /std:c++17 /nologo %OPT_FLAG% /MD %INCLUDES% %SOURCES% /Fe%OUT_DIR%/chip8.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console
cp SDL2.dll %OUT_DIR%