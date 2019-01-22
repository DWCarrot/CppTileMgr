@echo off

SET app=E:\workspace\CppTileMgr\x64\Debug\CppTileMgr

SET w=512
SET h=512
SET maxZ=5
SET minZ=0
SET p=E:\workspace\AKMDataManager\{z}\{x},{y}.png


SET args=-W%w% -H%h% -B%maxZ% -T%minZ% -p %p% -m2

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