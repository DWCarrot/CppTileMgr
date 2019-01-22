
#include "pch.h"
#include "TileManager.h"
#include "Util.h"

int parseRGBTuple(RGBTuple & res, const std::string & s)
{
	if (s.empty())
		return -1;
	const char *p = (char*)&s[0];
	char *end;
	if (*p != '{')
		return -1;
	++p;
	res.r = strtol(p, &end, 10);
	if (p == end || *end != ',')
		return -1;
	p = end + 1;
	res.g = strtol(p, &end, 10);
	if (p == end || *end != ',')
		return -1;
	p = end + 1;
	res.b = strtol(p, &end, 10);
	if (p == end || *end != '}')
		return -1;
	p = end + 1;
	if (*p != '\0')
		return -1;
	return 0;
}

int parseRGBTuple2(RGBTuple & res, const std::string & s)
{
	if (s.empty())
		return -1;
	const char *p = &s[0];
	char *end;
	if (*p != '#')
		return -1;
	p++;
	int value = strtol(p, &end, 16);
	if (p == end || *end != '\0' || value < 0 || value > 0xFFFFFF)
		return -1;
	res.r = (value >> 16) & 0xFF;
	res.g = (value >> 8) & 0xFF;
	res.b = (value >> 0) & 0xFF;
	return 0;
}

int parseXY(int & x, int & y, const std::string & s)
{
	if (s.empty())
		return -1;
	const char *p = (char*)&s[0];
	char *end;
	x = strtol(p, &end, 10);
	if (p == end || *end != ',')
		return -1;
	p = end + 1;
	y = strtol(p, &end, 10);
	if (p == end || *end != '\0')
		return -1;
	return 0;
}

bool operator<(const TileId & a, const TileId & b)
{
	if (a.z == b.z)
	{
		if (a.x == b.x)
		{
			if (a.y == b.y)
			{
				return false;
			}
			return a.y < b.y;
		}
		return a.x < b.x;
	}
	return a.z < b.z;
}

bool operator==(const TileId & a, const TileId & b)
{
	return a.z == b.z && a.x == b.x && a.y == b.y;
}

bool operator>(const TileId & a, const TileId & b)
{
	if (a.z == b.z)
	{
		if (a.x == b.x)
		{
			if (a.y == b.y)
			{
				return false;
			}
			return a.y > b.y;
		}
		return a.x > b.x;
	}
	return a.z > b.z;
}



TileId::TileId()
{
	z = 0;
	x = 0;
	y = 0;
//	printf("new: 0x%I64X\n\r", (uint64_t)this);
}

TileId::TileId(int z, int x, int y)
{
	this->z = z;
	this->x = x;
	this->y = y;
//	printf("new: 0x%I64X (%d, (%d, %d))\n\r", (uint64_t)this, z, x, y);
}

TileId::TileId(const TileId & id)
{
	z = id.z;
	x = id.x;
	y = id.y;
//	printf("copy: 0x%I64X <= 0x%I64X\n\r", (uint64_t)this, (uint64_t)&id);
}

TileId::TileId(TileId && id)
{
	z = id.z;
	x = id.x;
	y = id.y;
}

TileId & TileId::operator=(const TileId & id)
{
	z = id.z;
	x = id.x;
	y = id.y;
	return *this;
}

TileId & TileId::operator=(TileId && id)
{
	z = id.z;
	x = id.x;
	y = id.y;
	return *this;
}

TileId::~TileId()
{
//	printf("release: 0x%I64X (%d, (%d, %d))\n\r", (uint64_t)this, z, x, y);
	
}

TileId TileId::getZoomIn(int direction) const
{
	return TileId(z + 1, (x << 1) | (direction & 1), (y << 1) | ((direction & 2) >> 1));
}

TileId TileId::getZoomOut() const
{
	return TileId(z - 1, x >> 1, y >> 1);
}

int TileId::directionTo(const TileId & id) const
{
	if (z - id.z != 1)
		return -1;
	int dx = x - (id.x << 1);
	int dy = y - (id.y << 1);
	if (((dx | dy) & (~0x1)) != 0x0)
		return -1;
	return (dy << 1) | dx;
}





TileManager::TileManager(int minZoom, int maxZoom, int tileW, int tileH)
{
	if (minZoom <= maxZoom)
	{
		minZ = minZoom;
		maxZ = maxZoom;
		tw = tileW;
		th = tileH;
		transparent = { 0,0,0 };
		edgeOpt = STBImage::ScaleOpts::EDGE_CLAMP;
		filterOpt = STBImage::ScaleOpts::FILTER_DEFAULT;
	}
}


void TileManager::setGeo(int minZoom, int maxZoom, int tileW, int tileH)
{
	if (minZoom <= maxZoom)
	{
		minZ = minZoom;
		maxZ = maxZoom;
		tw = tileW;
		th = tileH;
	}
}

void TileManager::setEdgeOpt(STBImage::ScaleOpts edgeOpt)
{
	this->edgeOpt = edgeOpt;
}

void TileManager::setFilterOpt(STBImage::ScaleOpts filterOpt)
{
	this->filterOpt = filterOpt;
}

void TileManager::add(TileId tile)
{
	if (tile.z == maxZ && tile.z >= minZ)
	{
		while (tile.z >= minZ)
		{
			auto p = list.insert(tile);
			if (!p.second)
				break;
			tile = tile.getZoomOut();
		}
	}
}

void TileManager::route(TileId root, std::function<int(const TileId & id, const std::set<TileId>& list)>& callback)
{
	auto end = list.end();
	auto leaf = root;
	auto zlmt = root.z;
	while (true)	
	{
		// quad-tree iteration of DP
		//zoomIn: to child;	zoomOut: to parent;
		if (leaf.z > maxZ || list.find(leaf) == end)
		{
			root = leaf.getZoomOut();
			while(root.z >= minZ)
			{
				int direction = leaf.directionTo(root);
				if (direction >= 0 && direction < 3)
				{
					leaf = root.getZoomIn(direction + 1);
					break;
				}
				leaf = root;
				callback(leaf, list);
				root = leaf.getZoomOut();
			}
			if (root.z < zlmt)
				break;
		}
		else
		{
			leaf = leaf.getZoomIn(0);
		}
	}	
}

std::string TileManager::buildPath(const TileId & id, bool ensure)
{
	std::string res;
	int v[3];
	if (order[2] >= 0 && order[1] >= 0 && order[0] >= 0)
	{
		v[order[0]] = id.x;
		v[order[1]] = id.y;
		v[order[2]] = id.z;
		res.resize(pathpart[0].length() + pathpart[1].length() + pathpart[2].length() + pathpart[3].length() + 15 * 3 + 1);
		snprintf(
			&res[0], res.length(),
			"%s%d%s%d%s%d%s", 
			&pathpart[0][0],
			v[0],
			&pathpart[1][0],
			v[1], 
			&pathpart[2][0],
			v[2], 
			&pathpart[3][0]
		);
		res.resize(strlen(&res[0]));
		if (ensure)
		{
			int u = res.rfind(Miscellaneous::split);
			if (u > 0)
				Miscellaneous::mkdirs(res.substr(0, u));
		}
	}
	return res;
}

void TileManager::update(TileId root)
{
	std::function<int(const TileId&, const std::set<TileId>&)> cb 
		= std::bind(&TileManager::updateTile, this, std::placeholders::_1, std::placeholders::_2);
	route(root, cb);
}

int TileManager::updateTile(const TileId & id, const std::set<TileId>& ls)
{
	if (id.z == maxZ)
	{
		/*STBImage s = STBImage().load(buildPath(id));
		if (!s.isEmpty() && s.getC() >= 3 && s.getC() <= 4 && s.getW() == tw && s.getH() == th)
		{
			cache.emplace(std::make_pair(id, s));
		}*/
		return 0;
	}
	STBImage root(tw * 2, th * 2, 3);
	int ef = 0;
	for (int d = 0; d < 4; ++d)
	{
		int offx = (d & 1) * tw;
		int offy = ((d >> 1) & 1) * th;
		TileId leaf = id.getZoomIn(d);
		auto it = cache.find(leaf);
		STBImage s;
		if (it == cache.end())
		{
			s.load(buildPath(leaf));
		}
		else
		{
			s = it->second;
			cache.erase(it);
		}
		if (s.isEmpty() && root.getC() == 3)
			root = root.appendAlpha(transparent);
		if (!s.isEmpty() && s.getC() >= 3 && s.getC() <= 4 && s.getW() == tw && s.getH() == th)
		{
			if (s.getC() < root.getC())
				s = s.appendAlpha(transparent);
			else if (s.getC() > root.getC())
				root = root.appendAlpha(transparent);
			root.set(s, 0, 0, s.getW(), s.getH(), offx, offy);
			s.clear();
			ef++;
		}
	}
	if (ef == 0 || root.getC() < 3)
		return 0;

	if (!root.hasColor(transparent) && root.getC() == 4)
		root = root.removeAlpha();
	root = root.scale(
		tw, th, 
		STBImage::ScaleOpts::FILTER_DEFAULT, 
		STBImage::ScaleOpts::EDGE_CLAMP,
		STBImage::ResizeFlag_ALPHA_PREMULTIPLIED
	);
	if(!root.allColor(transparent))
		root.save(buildPath(id));
	if(id.z < maxZ)
		cache.emplace(std::make_pair(id, root));
	return 0;
}


TileManager::~TileManager()
{
	
}

std::set<TileId>& TileManager::getList()
{
	return list;
}

std::map<TileId, STBImage>& TileManager::getCache()
{
	return cache;
}

int TileManager::zmin() const
{
	return minZ;
}

int TileManager::zmax() const
{
	return maxZ;
}

int TileManager::tileW() const
{
	return tw;
}

int TileManager::tileH() const
{
	return th;
}

RGBTuple & TileManager::getTransparent()
{
	return transparent;
}

bool TileManager::pathInited()
{
	return (order[0] | order[1] | order[2]) >= 0;
}

bool TileManager::needCheckDir()
{
	return checkdir;
}

void TileManager::setCheckDir(bool checkdir)
{
	this->checkdir = checkdir;
}

void TileManager::setTransparent(const RGBATuple & transparent)
{
	memcpy(&this->transparent, &transparent, sizeof(transparent));
}

bool TileManager::registerPath(const std::string & pattern)
{
	memset(order, -1, sizeof(order));
	std::string fullpattern = Miscellaneous::absolute(pattern);
	int part = 0;
	int i, j, end;
	for (i = 0, j = i, end = fullpattern.length() - 3; i < end; ++i)
	{
		if (fullpattern[i] == '{' && fullpattern[i + 2] == '}')
		{
			char v = fullpattern[i + 1];
			if (v >= 'x' && v <= 'z')
			{
				pathpart[part] = fullpattern.substr(j, i - j);
				order[(int)(v - 'x')] = part;
				part++;
				j = i + 3;
				i = i + 2;
			}
		}
	}
	if(part < 3 || j > fullpattern.length())
		return false;
	pathpart[part] = fullpattern.substr(j);
	return true;
}

bool TileManager::checkTileSize(const STBImage & img)
{
	return img.getW() == tw && img.getH() == th && (img.getC() == 3 || img.getC() == 4);
}


const char *EmptyString = "";

int _m_getShortOpt_search(char tgt, const char *options)
{
	int type;
	for (const char *p = options; *p != '\0'; ++p)
	{
		if (*p == ':')
			continue;
		if (*p == tgt)
		{
			type = 0;
			if (*(p + 1) == ':')
			{
				type = 1;
				if (*(p + 2) == ':')
				{
					type = 2;
				}
			}
			return type;
		}
	}
	return -1;
}

int m_getShortOpt(char *opt, const char **optarg, int argc, char *const *argv, const char *options, int *p_argi)
{
	if (argv == nullptr)
		return -1;
	if (*p_argi >= argc)
		return -1;
	char *arg = argv[(*p_argi)++];
	if (arg[0] == '-' || arg[0] == '+')
	{
		*opt = arg[1];
		switch (_m_getShortOpt_search(*opt, options))
		{
		case 0:
			if (arg[2] == '\0')
			{
				*optarg = EmptyString;
			}
			else
			{
				*opt = '?';
				*optarg = arg;
			}
			break;
		case 1:
			if (arg[2] == '\0')
			{
				if (*p_argi < argc)
				{
					*optarg = argv[(*p_argi)++];
				}
				else
				{
					*optarg = NULL;
				}
			}
			else
			{
				*optarg = arg + 2;
			}
			break;
		case 2:
			if (arg[2] == '\0')
			{
				*optarg = EmptyString;
			}
			else
			{
				*optarg = arg + 2;
			}
			break;
		default:
			if(isalpha(arg[1]))
			{
				*opt = '?';
				*optarg = arg;
				break;
			}
			else
			{
				*opt = '\0';
				*optarg = arg;
			}
		}
	}
	else
	{
		*opt = '\0';
		*optarg = arg;
	}
	return 0;
}

void usage()
{
	
}

int demo(int argc, char * const argv[])
{
	int tw = 0;	//W
	int th = 0;	//H
	int minZ = 0;	//T
	int maxZ = 0;	//B
	RGBTuple transparent = { 0, 0, 0 };	//c
	STBImage::ScaleOpts edgeOpt = STBImage::ScaleOpts::EDGE_CLAMP;	//e
	STBImage::ScaleOpts filterOpt = STBImage::ScaleOpts::FILTER_DEFAULT;	//f
	std::string pattern;	//p
	int mode = 3;	//m		only-cover:1	only-update:2	all:3
	bool cheakDir = false;	//x
	std::vector<std::string> args;
	int stri = 0;
	int argi = 0;
	char opt;
	const char *optarg;
	char *end;
	while (m_getShortOpt(&opt, &optarg, argc, argv ,"hW:H:T:B:c:e:f:p:m:x", &argi) != -1)
	{
		switch (opt)
		{
		case 'h':
			usage();
			return 0;
		case 'W':
			tw = strtol(optarg, &end, 10);
			if (tw <= 0)
			{
				puts("illegal argument: tile width (\"-w\")");
				return 0;
			}
			break;
		case 'H':
			th = strtol(optarg, &end, 10);
			if (th <= 0)
			{
				puts("illegal argument: tile height (\"-h\")");
				return 0;
			}
			break;
		case 'T':
			minZ = strtol(optarg, &end, 10);
			if (optarg == end)
			{
				puts("illegal argument: minZoom (top-layer-zoom \"-t\")");
				return 0;
			}
			break;
		case 'B':
			maxZ = strtol(optarg, &end, 10);
			if (optarg == end)
			{
				puts("illegal argument: maxZoom (bottom-layer-zoom \"-b\")");
				return 0;
			}
			break;
		case 'c':
			if (parseRGBTuple2(transparent, optarg) != 0)
			{
				transparent = { 0, 0, 0 };
				puts("illegal argument: transparent color (\"-c\")");
				return 0;
			}
			break;
		case 'e':
			do
			{
				if (strcmp(optarg, "CLAM") == 0)
				{
					edgeOpt = STBImage::ScaleOpts::EDGE_CLAMP;	break;
				}
				if (strcmp(optarg, "REFLECT") == 0)
				{
					edgeOpt = STBImage::ScaleOpts::EDGE_REFLECT;	break;
				}
				if (strcmp(optarg, "WRAP") == 0)
				{
					edgeOpt = STBImage::ScaleOpts::EDGE_WRAP;	break;
				}
				if (strcmp(optarg, "ZERO") == 0)
				{
					edgeOpt = STBImage::ScaleOpts::EDGE_ZERO;	break;
				}
				puts("illegal argument: scale-edge option (\"-e\")");
				return 0;
			} while (false);
			break;
		case 'f':
			do
			{
				if (strcmp(optarg, "DEFAULT") == 0)
				{
					filterOpt = STBImage::ScaleOpts::FILTER_DEFAULT;	break;
				}
				if (strcmp(optarg, "BOX") == 0)
				{
					filterOpt = STBImage::ScaleOpts::FILTER_BOX;	break;
				}
				if (strcmp(optarg, "TRIANGLE") == 0)
				{
					filterOpt = STBImage::ScaleOpts::FILTER_TRIANGLE;	break;
				}
				if (strcmp(optarg, "CUBICBSPLINE") == 0)
				{
					filterOpt = STBImage::ScaleOpts::FILTER_CUBICBSPLINE;	break;
				}
				if (strcmp(optarg, "CATMULLROM") == 0)
				{
					filterOpt = STBImage::ScaleOpts::FILTER_CATMULLROM;	break;
				}
				if (strcmp(optarg, "FILTER_MITCHELL") == 0)
				{
					filterOpt = STBImage::ScaleOpts::FILTER_MITCHELL;	break;
				}
				puts("illegal argument: scale-interpolation option (\"-f\")");
				return 0;
			} while (false);
			break;
		case 'p':
			pattern = std::string(optarg);
			break;
		case 'm':
			mode = strtol(optarg, &end, 10);
			if(optarg == end || mode < 1 || mode > 3)
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

	if(tw <= 0)
	{
		puts("illegal argument: tile width (\"-w\")");
		return 0;
	}
	if (th <= 0)
	{
		puts("illegal argument: tile height (\"-h\")");
		return 0;
	}
	if (minZ > maxZ)
	{
		puts("illegal argument: minZoom (top-layer-zoom \"-t\") or maxZoom (bottom-layer-zoom \"-b\")");
		return 0;
	}
	TileManager mgr(minZ, maxZ, tw, th);
	if (!mgr.registerPath(pattern))
	{
		puts("illegal argument: path pattern (\"-p\")");
		return 0;
	}

	mgr.setEdgeOpt(edgeOpt);
	mgr.setFilterOpt(filterOpt);
	mgr.setCheckDir(cheakDir);

	Timer timer;

	if ((mode & 1) == 1)
	{
		printf("==== cover tiles ====\n");
		for (int i = 0; i < args.size(); i += 2)
		{
			timer.start();
			TileId tile;
			tile.z = mgr.zmin();
			if (parseXY(tile.x, tile.y, args.at(i)) != 0)
				continue;
			std::string path = mgr.buildPath(tile, mgr.needCheckDir());
			STBImage img1, img2;
			img1.load(path);
			img2.load(args.at(i + 1));
			if (!(mgr.checkTileSize(img1) && mgr.checkTileSize(img2)))
				continue;
			size_t c;
			if (STBImage::cover_rgba(img1, 0, 0, img1, 0, 0, img2, 0, 0, mgr.tileW(), mgr.tileH(), mgr.getTransparent(), c))
				continue;
			if (c == 0 && img1.getC() == 4)
				img1 = img1.removeAlpha();
			if (img1.save(path).getException())
				continue;
			timer.stop();
			printf("\tTile (%d,(%d,%d)) coverd [by %s]\t\t\t%fs\n", tile.z, tile.x, tile.y, args.at(i + 1).c_str(), timer.get());
		}
		printf("====  ====\n");
	}
	if ((mode & 2) == 2)
	{
		int itv = (mode & 1) + 1;
		printf("==== update tiles ====\n");
		for (int i = 0; i < args.size(); i += itv)
		{
			TileId tile;
			tile.z = mgr.zmax();
			if (parseXY(tile.x, tile.y, args.at(i)) != 0)
				continue;
			mgr.add(tile);
			printf("\tadd task: tile (%d,(%d,%d))\n", tile.z, tile.x, tile.y);
		}
		puts("");
		for (auto i : mgr.getList())
		{
			if (i.z > mgr.zmin())
				break;
			timer.start();
			mgr.update(i);
			timer.stop();
			printf("\tcomplete branch  with top tile (%d,(%d,%d))\t\t\t%fs\n", i.z, i.x, i.y, timer.get());
		}
		printf("====  ====\n");
	}

	return 0;
}



