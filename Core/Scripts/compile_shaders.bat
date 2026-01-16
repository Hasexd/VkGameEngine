@echo off
for %%f in (Shaders\*.frag Shaders\*.vert Shaders\*.comp Shaders\*.geom) do (
    glslang "%%f" -V -o Shaders\Compiled\%%~nxf.spv
)