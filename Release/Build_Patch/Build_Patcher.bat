@echo off
setlocal
color 0A
title Zombie Tsunami - Build Patcher
cd /d "%~dp0"

:MENU
cls
echo ==================================================================
echo          ZOMBIE TSUNAMI - BUILD DO PATCHER PARA PS VITA
echo ==================================================================
echo.
echo   [1] Recompilar o executavel usando o patch_data atual
echo   [2] Atualizar patch_data e recompilar o executavel
echo   [0] Sair
echo.
set /p CHOICE="  Opcao: "
if "%CHOICE%"=="1" goto FIND_PYTHON
if "%CHOICE%"=="2" goto FIND_PYTHON
if "%CHOICE%"=="0" exit /b 0
echo.
echo   Opcao invalida.
pause
goto MENU

:FIND_PYTHON
where python >nul 2>nul
if not errorlevel 1 (
  set "PYTHON=python"
  goto CHECK_MODULES
)
where py >nul 2>nul
if not errorlevel 1 (
  set "PYTHON=py -3"
  goto CHECK_MODULES
)
echo.
echo   ERRO: Python 3 nao foi encontrado.
pause
exit /b 1

:CHECK_MODULES
%PYTHON% -c "import bsdiff4, PyInstaller" >nul 2>nul
if errorlevel 1 (
  echo.
  echo   ERRO: instale as dependencias com:
  echo   pip install bsdiff4 pyinstaller
  pause
  exit /b 1
)

if "%CHOICE%"=="2" (
  echo.
  echo   [1/2] Atualizando as diferencas binarias...
  %PYTHON% "src\build_patch_data.py"
  if errorlevel 1 goto FAILED
)

echo.
echo   Compilando ZombieTsunamiPatcher.exe...
pushd src
%PYTHON% -m PyInstaller --clean --noconfirm ZombieTsunamiPatcher.spec
if errorlevel 1 (
  popd
  goto FAILED
)
popd

copy /y "src\dist\ZombieTsunamiPatcher.exe" "..\Patcher\ZombieTsunamiPatcher.exe" >nul
if errorlevel 1 goto FAILED
echo.
echo   SUCESSO: ..\Patcher\ZombieTsunamiPatcher.exe foi atualizado.
pause
exit /b 0

:FAILED
echo.
echo   ERRO: o build nao foi concluido.
pause
exit /b 1
