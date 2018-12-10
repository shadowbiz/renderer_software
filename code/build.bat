@echo off

REM call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64

set CommonCompilerFlags=-MTd -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DSHADOWBIZ_INTERNAL=1 -DSHADOWBIZ_SLOW=1 -DSHADOWBIZ_WIN32=1 -FC -Z7
set CommonLinkerFlags= -opt:ref user32.lib gdi32.lib winmm.lib

REM TODO - can we just build both with one exe?

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM 32-bit build
REM cl %CommonCompilerFlags% handmade\code\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
del *.pdb > NUL 2> NUL
REM Optimization switches /O2

cl %CommonCompilerFlags% ..\code\win32_shadowbiz.cpp -Fmwin32_shadowbiz.map /link %CommonLinkerFlags%
REM echo WAITING FOR PDB > lock.tmp
REM cl %CommonCompilerFlags% ..\shadowbiz\code\shadowbiz.cpp -Fmshadowbiz.map -LD /link -incremental:no -opt:ref -PDB:shadowbiz_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
REM del lock.tmp
REM cl %CommonCompilerFlags% ..\shadowbiz\code\win32_shadowbiz.cpp -Fmwin32_shadowbiz.map /link %CommonLinkerFlags%
popd
