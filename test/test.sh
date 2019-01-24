#!/bin/sh


app="../CppTileMgr"

w=256
h=256
maxZ=5
minZ=1
p="$HOME/workspace/cpp/tilemgr/pics/z{z}/tile/{x},{y}.png"



read -r -p "download [Y/n] " input
if [ "$input" = "Y" ]; then
	urlb="https://dwcarrot.github.io/AlternativeKedamaUnofficialMap/data/v3/4/"
	for x in $(seq -8 7); do
        	for y in $(seq -8 7); do
			echo "$urlb/$x,$y.png" >> download.list
        	done
	done
	wget -P ../pics/z5/tile -i download.list
fi

args="-W$w -H$h -B$maxZ -T$minZ -p $p -m2 -x"

for x in $(seq -8 7); do 
	for y in $(seq -8 7); do
		args="$args $x,$y"
	done
done

echo $app $args

$app $args

args="-W$w -H$h -B$maxZ -T$minZ -p $p -m3"
args="$args -6,2 ../pics/tmp/-6,2.bmp 6,6 ../pics/tmp/6,6.png"

echo $app $args

$app $args
