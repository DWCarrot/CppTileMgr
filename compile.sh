#!/bin/sh

src="$(pwd)/src"
tgt="$(pwd)/Build"
cc="clang++"
std="-std=c++11"
stdlib="-stdlib=libc++"
lib="-l pthread"
optm="-O3"
#marco="-D _DEBUG"


mkdir Build
cd Build

echo
echo $cc $std $stdlib $lib $optm $marco -shared -fPIC $src/pch.cpp $src/STBImage.cpp $src/TileManager.cpp $src/implement.cpp -o $tgt/libtilemgr.so
echo
$cc $std $stdlib $lib $optm $marco -shared -fPIC $src/pch.cpp $src/STBImage.cpp $src/TileManager.cpp $src/implement.cpp -o $tgt/libtilemgr.so

echo
echo $cc $std $stdlib $lib $optm -o $tgt/CppTileMgr $src/Ctilemgr.cpp $tgt/libtilemgr.so
echo
$cc $std $stdlib $lib $optm -o $tgt/CppTileMgr $src/Ctilemgr.cpp $tgt/libtilemgr.so

cd ../
mv $tgt/CppTileMgr ./
export LD_LIBRARY_PATH=$tgt:$LD_LIBRARY_PATH
echo $LD_LIBRARY_PATH
