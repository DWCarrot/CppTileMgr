
#include "pch.h"
#include "TileManager.h"


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



TileManager::TileManager()
{
	minZ = 0;
	maxZ = 0;
	tw = 0;
	th = 0;
	checkdir = false;
	transparent = { 0,0,0 };
	edgeOpt = STBImage::ScaleOpts::EDGE_CLAMP;
	filterOpt = STBImage::ScaleOpts::FILTER_DEFAULT;

	memset(order, -1, sizeof(order));

	list.clear();
}

TileManager::TileManager(int minZoom, int maxZoom, int tileW, int tileH)
{
	
	if (minZoom <= maxZoom)
	{
		setGeo(minZoom, maxZoom, tileW, tileH);
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

void TileManager::update(TileId root, std::map<TileId, STBImage> & cache)
{
	std::function<int(const TileId&, const std::set<TileId>&)> cb 
		= std::bind(&TileManager::updateTile, this, cache, std::placeholders::_1, std::placeholders::_2);
	route(root, cb);
}

void TileManager::update(TileId root)
{
	std::map<TileId, STBImage> cache;
	update(root, cache);
}

int TileManager::updateTile(std::map<TileId, STBImage> & cache, const TileId & id, const std::set<TileId>& ls)
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
	STBImage root(tw * 2, th * 2, 4);
	int ef = 0;
	size_t ct = 0;
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
			s = std::move(it->second);
			cache.erase(it);
		}	
		/*if (s.isEmpty() && root.getC() == 3)
			root = root.appendAlpha(transparent);*/
		if (!s.isEmpty() && checkTileSize(s) && s.getC() >= 3 && s.getC() <= 4 && s.getW() == tw && s.getH() == th)
		{
			/*if (s.getC() < root.getC())
				s = s.appendAlpha(transparent);
			root.set(s, 0, 0, s.getW(), s.getH(), offx, offy);*/
			set(root, s, offx, offy, ct);
			s.clear();
			ef++;
		}
	}
	if (ef == 0 || root.getC() < 3)
		return 0;

	if (ef == 4 && ct == 0)
		root = root.removeAlpha();
	root = root.scale(
		tw, th, 
		STBImage::ScaleOpts::FILTER_DEFAULT, 
		STBImage::ScaleOpts::EDGE_CLAMP,
		STBImage::ResizeFlag_ALPHA_PREMULTIPLIED
	);
	root.save(buildPath(id, checkdir));
	if(id.z < maxZ)
	{
		cache.emplace(std::make_pair(id, root));
	}
		
	return 0;
}

int TileManager::set(STBImage & tgt, STBImage & src, int offx, int offy, size_t & count)
{
	int i, j;
	const int bwT = tgt.getC(), bwS = src.getC(), lwT = bwT * tgt.getW(), lwS = bwS * src.getW();
	const bool alphaT = tgt.getC() == 4, alphaS = src.getC() == 4;
	bool tr, tg, tb, t;
	uint8_t *pT, *pTL, *pS, *pSL, alpha;
	for (
		j = 0, pTL = tgt.getData() + offy * lwT, pSL = src.getData();
		j < th;
		j += 1, pTL += lwT, pSL += lwS
		)
	{
		for (
			i = 0, pT = pTL + offx * bwT, pS = pSL;
			i < tw;
			i += 1, pT += bwT, pS += bwS
			)
		{
			tr = (pT[0] = pS[0]) != transparent.r;
			tg = (pT[1] = pS[1]) != transparent.g;
			tb = (pT[2] = pS[2]) != transparent.b;
			alpha = alphaS ? pS[3] : (0xFF * (tr || tg || tb));
			if (alphaT)
				pT[3] = alpha;
			count += (int)(alpha != 0xFF);
		}
	}
	return 0;
}

int TileManager::coverTile(int x, int y, const std::string & pic)
{
	TileId tile(maxZ, x, y);
	STBImage img1, img2;
	std::string old = buildPath(tile);
	img2.load(pic);
	if (img2.getException() & STBImage::EXCEPTION_C_LOAD_FAIL)
		return STBImage::EXCEPTION_C_LOAD_FAIL;
	if (old.empty())
		return 0x010000;
	img1.load(old);
	if (img1.getException() & STBImage::EXCEPTION_C_LOAD_FAIL)
	{
		img1.clearException(STBImage::EXCEPTION_C_LOAD_FAIL);
		img1.allocate(img1.getW(), img1.getH(), 4);
	}
	if (!(checkTileSize(img1) && checkTileSize(img2)))
		return STBImage::EXCEPTION_SIZE;
	if (img1.getC() < 3 || img2.getC() < 3)
		return STBImage::EXCEPTION_CHANNEL;
	if (img1.getC() < img2.getC() || img2.hasColor(transparent))
		img1 = img1.appendAlpha(transparent);

	int i;
	const int bw1 = img1.getC(), bw2 = img2.getC();
	const bool alpha1 = (bw1 == 4), alpha2 = (bw2 == 4);
	uint8_t  *p1, *p2;

	const int sz = tw * th;

	size_t transparent_count = 0;

	for (i = 0, p1 = img1.getData(), p2 = img2.getData();
		i < sz;
		i += 1, p1 += bw1, p2 += bw2
		) {

		if ((p2[0] == transparent.r && p2[1] == transparent.g && p2[2] == transparent.b) || (alpha2 && p2[3] == 0))
		{
			
		}
		else
		{
			p1[0] = p2[0];
			p1[1] = p2[1];
			p1[2] = p2[2];
			if (alpha1)
				p1[3] = 255;
		}
		if ((p1[0] == transparent.r && p1[1] == transparent.g && p1[2] == transparent.b) || (alpha1 && p1[3] == 0))
		{
			transparent_count++;
		}
	}

	if (img1.getC() > 3 && transparent_count == 0)
		img1 = img1.removeAlpha();

	img1.save(old, checkdir);
	return img1.getException();
}




void TileManager::clear()
{
	list.clear();
}



TileManager::~TileManager()
{
	
}

std::set<TileId>& TileManager::getList()
{
	return list;
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

bool TileManager::geoInited()
{
	return tw > 0 && th > 0 && minZ <= maxZ;
}

bool TileManager::needCheckDir()
{
	return checkdir;
}

void TileManager::setCheckDir(bool checkdir)
{
	this->checkdir = checkdir;
}

void TileManager::setTransparent(const RGBTuple & transparent)
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







