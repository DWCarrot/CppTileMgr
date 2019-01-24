@echo off

SET app=E:\workspace\CppTileMgr\x64\Debug\CppTileMgr

SET w=256
SET h=256
SET maxZ=5
SET minZ=1
SET p=E:\workspace\AKMDataManager\{z}\{x},{y}.png


SET args=-W%w% -H%h% -B%maxZ% -T%minZ% -p %p% -m2 -x

setlocal enabledelayedexpansion
for /l %%x in (-10,1,9) do (
	for /l %%y in (-10,1,9) do (
		SET args=!args! %%x,%%y
	)
)
setlocal disabledelayedexpansion

echo %app% %args%

pause

%app% %args%

pause

SET args=-W%w% -H%h% -B%maxZ% -T%minZ% -p %p% -m3 -6,2 E:\workspace\AKMDataManager\tmp\-6,2.bmp 6,6 E:\workspace\AKMDataManager\tmp\6,6.png

echo %app% %args%

pause
%app% %args%

pause