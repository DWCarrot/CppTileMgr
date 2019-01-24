#include "implement.h"
#include <string.h>
#include <stdlib.h>


int i_setParam(int tw, int th, int minZoom, int maxZoom, const char * pattern)
{
	if (tw <= 0 || th <= 0 || minZoom > maxZoom)
		return EX_Invalid_Argument;
	mgr.setGeo(minZoom, maxZoom, tw, th);
	if(!mgr.registerPath(pattern))
		return EX_Invalid_Argument;
	return 0;
}

int i_setParam_edgeOpt(const char * edgeOpt)
{
	do
	{
		if (strcmp(edgeOpt, "CLAM") == 0)
		{
			mgr.setEdgeOpt(STBImage::ScaleOpts::EDGE_CLAMP);	break;
		}
		if (strcmp(edgeOpt, "REFLECT") == 0)
		{
			mgr.setEdgeOpt(STBImage::ScaleOpts::EDGE_REFLECT);	break;
		}
		if (strcmp(edgeOpt, "WRAP") == 0)
		{
			mgr.setEdgeOpt(STBImage::ScaleOpts::EDGE_WRAP);	break;
		}
		if (strcmp(edgeOpt, "ZERO") == 0)
		{
			mgr.setEdgeOpt(STBImage::ScaleOpts::EDGE_ZERO);	break;
		}
		return EX_Invalid_Argument;
	} while (false);
	return 0;
}

int i_setParam_interpolationOpt(const char * filterOpt)
{
	do
	{
		if (strcmp(filterOpt, "DEFAULT") == 0)
		{
			mgr.setFilterOpt(STBImage::ScaleOpts::FILTER_DEFAULT);	break;
		}
		if (strcmp(filterOpt, "BOX") == 0)
		{
			mgr.setFilterOpt(STBImage::ScaleOpts::FILTER_BOX);	break;
		}
		if (strcmp(filterOpt, "TRIANGLE") == 0)
		{
			mgr.setFilterOpt(STBImage::ScaleOpts::FILTER_TRIANGLE);	break;
		}
		if (strcmp(filterOpt, "CUBICBSPLINE") == 0)
		{
			mgr.setFilterOpt(STBImage::ScaleOpts::FILTER_CUBICBSPLINE);	break;
		}
		if (strcmp(filterOpt, "CATMULLROM") == 0)
		{
			mgr.setFilterOpt(STBImage::ScaleOpts::FILTER_CATMULLROM);	break;
		}
		if (strcmp(filterOpt, "FILTER_MITCHELL") == 0)
		{
			mgr.setFilterOpt(STBImage::ScaleOpts::FILTER_MITCHELL);	break;
		}
		return EX_Invalid_Argument;
	} while (false);
	return 0;
}

int i_setParam_transparentColor(const char * colorStr)
{
	if (*colorStr == '#')
	{
		char *end;
		colorStr++;
		int32_t c = strtol(colorStr, &end, 16);
		if (end == colorStr || c < 0 || c > 0xFFFFFF)
			return EX_Invalid_Argument;
		RGBTuple c0;
		c0.r = (c >> 16) & 0xFF;
		c0.g = (c >> 8) & 0xFF;
		c0.b = (c >> 0) & 0xFF;
		mgr.setTransparent(c0);
		return 0;
	}
	if (*colorStr == '{')
	{
		RGBTuple c0;
		char *end;
		colorStr++;
		c0.r = strtol(colorStr, &end, 10);
		if (end == colorStr || *end != ',')
			return EX_Invalid_Argument;
		colorStr = end + 1;
		c0.g = strtol(colorStr, &end, 10);
		if (end == colorStr || *end != ',')
			return EX_Invalid_Argument;
		colorStr = end + 1;
		c0.b = strtol(colorStr, &end, 10);
		if (end == colorStr || *end != '}')
			return EX_Invalid_Argument;
		mgr.setTransparent(c0);
		return 0;
	}
	return EX_Invalid_Argument;
}

int i_setParam_checkDir(int checkDir)
{
	mgr.setCheckDir(checkDir);
	return 0;
}

int i_coverTile(int x, int y, const char * picFile)
{
	if(mgr.geoInited() && mgr.pathInited())
		return mgr.coverTile(x, y, picFile);
	return EX_UnInited;
}

int i_addTask(int x, int y)
{
	if (mgr.geoInited() && mgr.pathInited())
	{
		mgr.add(TileId(mgr.zmax(), x, y));
		return 0;
	}
	return EX_UnInited;
}

int i_update(int thread)
{
	if(!(mgr.pathInited() && mgr.geoInited()))
		return EX_UnInited;
	if (thread <= 1)
	{
		for (auto & id : mgr.getList())
		{
			if (id.z > mgr.zmin())
				break;
			mgr.update(id);
		}
	}
	else
	{
		
	}
	return 0;
}


int i_clear()
{
	mgr.clear();
	return 0;
}


int demo(int argc, char * const argv[])
{
	Timer timer;
	timer.start();
	int tw = 0;	//W
	int th = 0;	//H
	int minZ = 0;	//T
	int maxZ = 0;	//B
	const char *p_pattern = nullptr;	//p
	const char *p_transparent = nullptr;	//c
	const char *p_edgeOpt = nullptr;	//e
	const char *p_filterOpt = nullptr;	//f
	int mode = 3;	//m		only-cover:1	only-update:2	all:3
	bool cheakDir = false;	//x
	std::vector<const char*> args;
	int stri = 0;
	int argi = 0;
	char opt;
	const char *optarg;
	char *end;
	while (m_getShortOpt(&opt, &optarg, argc, argv, "hW:H:T:B:c:e:f:p:m:x", &argi) != -1)
	{
		switch (opt)
		{
		case 'h':

			return 0;
		case 'W':
			tw = strtol(optarg, &end, 10);
			break;
		case 'H':
			th = strtol(optarg, &end, 10);
			break;
		case 'T':
			minZ = strtol(optarg, &end, 10);
			break;
		case 'B':
			maxZ = strtol(optarg, &end, 10);
			break;
		case 'c':
			p_transparent = optarg;
			break;
		case 'e':
			p_edgeOpt = optarg;
			break;
		case 'f':
			p_filterOpt = optarg;
			break;
		case 'p':
			p_pattern = optarg;
			break;
		case 'm':
			mode = strtol(optarg, &end, 10);
			if (optarg == end || mode < 1 || mode > 3)
			{
				puts("illegal argument: mode (\"-m\")");
				return 0;
			}
			break;
		case 'x':
			cheakDir = true;
			break;
		case '\0':
			args.push_back(optarg);
			break;
		default:
			break;
		}
	}
	int error;
	if(error = i_setParam(tw, th, minZ, maxZ, p_pattern))
	{
		printf("Exception: %d\n", error);
		return 0;
	}
	if (p_edgeOpt != nullptr)
	{
		if (error = i_setParam_edgeOpt(p_edgeOpt))
		{
			printf("Exception: %d\n", error);
			return 0;
		}
	}
	if (p_filterOpt != nullptr)
	{
		if (error = i_setParam_interpolationOpt(p_edgeOpt))
		{
			printf("Exception: %d\n", error);
			return 0;
		}
	}
	if (error = i_setParam_checkDir(cheakDir))
	{
		printf("Exception: %d\n", error);
		return 0;
	}
	timer.stop();
	printf("\tInitialized in %fs\n", timer.get());
	

	if ((mode & 1) == 1)
	{
		printf("==== cover tiles ====\n");
		for (int i = 0; i < args.size(); i += 2)
		{
			timer.start();
			int x, y;
			if (parseXY(x, y, args.at(i)) != 0)
				continue;
			if (i_coverTile(x, y, args.at(i + 1)) != 0)
				continue;
			timer.stop();
			printf("\tTile (%d,(%d,%d)) coverd [by %s]\t\t\t%fs\n", mgr.zmax(), x, y, args.at(i + 1), timer.get());
			if ((mode & 2) == 2)
			{
				i_addTask(x, y);
				printf("\tadd task: tile (%d,(%d,%d))\n", mgr.zmax(), x, y);
			}
		}
		printf("====  ====\n");
	}
	else 
	{
		for (int i = 0; i < args.size(); i += 1)
		{
			int x, y;
			if (parseXY(x, y, args.at(i)) != 0)
				continue;
			i_addTask(x, y);
			printf("\tadd task: tile (%d,(%d,%d))\n", mgr.zmax(), x, y);
		}
	}

	puts("");
	
	int thread = 1;

	printf("==== update tiles ====\n");
	if (!(mgr.pathInited() && mgr.geoInited()))
		return EX_UnInited;
	if (thread <= 1)
	{
		for (auto & id : mgr.getList())
		{
			if (id.z > mgr.zmin())
				break;
			timer.start();
			mgr.update(id);
			timer.stop();
			printf("\tupdate branch with top tile(%d,(%d,%d)) in %fs\n", id.z, id.x, id.y, timer.get());
		}
	}
	else
	{

	}
	printf("====  ====\n");

	return 0;
}