@echo off
echo ========================================================
echo AETHERIS // C++/OpenGL Prototype Build Script (MSYS2 g++)
echo ========================================================
echo.

:: Check for MSYS2 g++
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] g++ compiler not found in system PATH.
    echo Please make sure MSYS2 or MinGW g++ is installed and on your PATH.
    echo.
    pause
    exit /b 1
)

echo [INFO] Compiling Space Debris Visualizer...
g++ main.cpp OrbitalDebris.cpp DensityGrid.cpp Renderer.cpp -o space_debris_sim.exe -O3 -Wall -lglfw3 -lgdi32 -lopengl32

if %errorlevel% equ 0 (
    echo [SUCCESS] Compilation completed successfully!
    echo Run 'space_debris_sim.exe' to launch the desktop prototype.
) else (
    echo [ERROR] Compilation failed.
    echo Please ensure the GLFW library is installed in your MSYS2 / Mingw environment:
    echo pacman -S mingw-w64-x86_64-glfw
)
echo.
pause
