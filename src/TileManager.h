#ifndef TILEMANAGER_H__
#define TILEMANAGER_H__

#include "STBImage.h"

#include <set>
#include <vector>
#include <map>

#include "Util.h"

//"{$r,$g,$b}"
int parseRGBTuple(RGBTuple & res, const std::string & s);

//"#RRGGBB"
int parseRGBTuple2(RGBTuple & res, const std::string & s);

//"$x,$y"
int parseXY(int & x, int & y, const std::string & s);


struct Point
{
	int x;
	int y;
};

class TileId
{

public:
	TileId();
	TileId(int z, int x, int y);
	TileId(const TileId & id);
	TileId(TileId && id);
	TileId & operator=(const TileId & id);
	TileId & operator=(TileId && id);
	~TileId();
	TileId getZoomIn(int direction) const;//	x->  y |V  0,1;2,3
	TileId getZoomOut() const;
	int directionTo(const TileId & id) const;	//child direction to father `id`
public:
	int z;
	int x;
	int y;
//	STBImage tile;
};

bool operator<(const TileId & a, const TileId & b);
bool operator==(const TileId & a, const TileId & b);
bool operator>(const TileId & a, const TileId & b);

class TileManager
{
private:

	int minZ;
	int maxZ;
	int tw;
	int th;
	RGBTuple transparent;
	STBImage::ScaleOpts edgeOpt;
	STBImage::ScaleOpts filterOpt;
private:
	std::set<TileId> list;
	std::map<TileId, STBImage> cache;
	
	std::string pathpart[4];
	int order[3];

	bool checkdir;

public:
	TileManager(int minZoom, int maxZoom, int tileW, int tileH);//minZoom, maxZoom can reach
	~TileManager();

	std::set<TileId>& getList();
	std::map<TileId, STBImage>& getCache();
	int zmin() const;
	int zmax() const;
	int tileW() const;
	int tileH() const;
	RGBTuple& getTransparent();

	bool pathInited();
	bool needCheckDir();

	void setCheckDir(bool checkdir);
	void setTransparent(const RGBATuple & transparent);

	void setGeo(int minZoom, int maxZoom, int tileW, int tileH);

	void setEdgeOpt(STBImage::ScaleOpts edgeOpt);

	void setFilterOpt(STBImage::ScaleOpts filterOpt);

	void add(TileId tile);
	void route(TileId root, std::function<int(const TileId & id, const std::set<TileId>& list)>& callback);

	std::string buildPath(const TileId & id, bool ensure = false);

	void update(TileId root);

	bool registerPath(const std::string & pattern);

	bool checkTileSize(const STBImage & img);

private:

	int updateTile(const TileId & id, const std::set<TileId>& list);

public:

};


int _m_getShortOpt_search(char tgt, const char *options);
int m_getShortOpt(char *opt, const char **optarg, int argc, char *const *argv, const char *options, int *p_argi);

int demo(int argc, char *const argv[]);

#endif // !TILEMANAGER_H__

