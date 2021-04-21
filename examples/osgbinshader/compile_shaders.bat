@echo off
for %%i in (*.vert *.frag) do "glslangValidator.exe" -G  "%%~i" -o "%%~i.spv"
pause