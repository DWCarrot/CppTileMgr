#ifndef IMPLEMENT_H__
#define IMPLEMENT_H__

#include "TileManager.h"

constexpr int EX_Invalid_Argument = 0x0002 << 16;
constexpr int EX_UnInited = 0x0004 << 16;

static TileManager mgr;

int demo(int argc, char *const argv[]);

//@return: 0:success; others: error

int i_setParam(int tw, int th, int minZoom, int maxZoom, const char *pattern);

int i_setParam_edgeOpt(const char *edgeOpt);

int i_setParam_interpolationOpt(const char *filterOpt);

int i_setParam_transparentColor(const char *colorStr);

int i_setParam_checkDir(int checkDir);//bool checkDir

int i_coverTile(int x, int y, const char *picFile);

int i_addTask(int x, int y);

int i_update(int thread);

int i_clear();

#endif
