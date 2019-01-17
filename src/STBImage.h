#ifndef STBIMAGE_H__
#define STBIMAGE_H__

#include <cstdint>
#include <string>
#include <functional>
#include <exception>


#ifdef _DEBUG

#include "Util.h"

static Memory mem_mgr;

#define STBI_MALLOC(sz) mem_mgr.rt_malloc(sz, __FILE__, __LINE__)
#define STBI_FREE(p)  mem_mgr.rt_free(p, __FILE__, __LINE__)
#define STBI_REALLOC(p, sz)mem_mgr.rt_realloc(p, sz, __FILE__, __LINE__)

#endif // DEBUG



struct RGBTuple
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct RGBATuple
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

struct unsafe_iterator
{
	int i;
	int j;
	uint8_t *next;
	int ch;
	int xmin;
	int xmax;
	int ymin;
	int ymax;
	uint8_t *_pL;
	uint8_t *_pC;
	int _lineWB;
};

static size_t default_transparent_count = 0;
static RGBTuple default_transparent_color = { 0,0,0 };

class STBImage
{

public:
	enum ScaleOpts
	{
		EDGE_CLAMP,
		EDGE_REFLECT,
		EDGE_WRAP,
		EDGE_ZERO,
		FILTER_DEFAULT,		// use same filter type that easy-to-use API chooses
		FILTER_BOX,			// A trapezoid w/1-pixel wide ramps, same result as box for integer scale ratios
		FILTER_TRIANGLE,	// On upsampling, produces same results as bilinear texture filtering
		FILTER_CUBICBSPLINE,// The cubic b-spline (aka Mitchell-Netrevalli with B=1,C=0), gaussian-esque
		FILTER_CATMULLROM,  // An interpolating cubic spline
		FILTER_MITCHELL,	// Mitchell-Netrevalli filter with B=1/3, C=1/3
	};

	enum ImageFmt
	{
		INVALID,
		BMP,
		JPEG,
		PNG,
	};

	static const int ResizeFlag_ALPHA_PREMULTIPLIED;
	static const int ResizeFlag_GAMMA_CORRECT;

public:
	static const uint16_t EXCEPTION_MEMORY			= 0x0001;
	static const uint16_t EXCEPTION_EMPTY			= 0x0002;
	static const uint16_t EXCEPTION_SIZE			= 0x0004;
	static const uint16_t EXCEPTION_CHANNEL			= 0x0008;
	static const uint16_t EXCEPTION_C_LOAD_FAIL		= 0x0010;
	static const uint16_t EXCEPTION_C_SAVE_FAIL		= 0x0020;
	static const uint16_t EXCEPTION_C_RESIZE_FAIL	= 0x0040;

	static const uint16_t EXCEPTION_ALLEXCEPTION	= 0xFFFF;

private:



protected:
	int w;
	int h;
	int ch;
	uint8_t *data;
	uint16_t exception;

public:
	STBImage();
	STBImage(int w, int h, int ch);
	STBImage(const STBImage & img);
	STBImage(STBImage && img);
	STBImage & operator=(const STBImage & img);
	STBImage & operator=(STBImage && img);
	~STBImage();
	bool allocate(int w, int h, int ch);
	void clear();
	void move(STBImage & img);
	inline int getW() const { return w; }
	inline int getH() const { return h; }
	inline int getC() const { return ch; }
	inline uint8_t *getData() const { return data; }
	inline bool isEmpty() const { return data == nullptr; }
	inline void setException(uint16_t exception) { this->exception |= exception; }
	inline void clearException(uint16_t exception) { this->exception &= (~exception); }
	inline void clearException() { this->exception = 0; }
	inline uint16_t getException() const { return this->exception; }
	inline const bool _checkboard(int xmin, int ymin, int xmax, int ymax) const { return (xmin >= 0 && ymin >= 0 && xmax <= w && ymax <= h && xmin <= xmax && ymin <= ymax); }


public:
	STBImage & load(const std::string & filename);
	STBImage & save(const std::string & filename, int argc = 0, int *argv = nullptr);
	STBImage appendAlpha(uint8_t alpha = 0xFF);
	STBImage appendAlpha(const RGBTuple & transparent);
	STBImage removeAlpha();
	STBImage removeAlpha(const RGBTuple & transparent);
	bool iterate(int xmin, int ymin, int xmax, int ymax, const std::function<bool(uint8_t *pixel, int ch, int x, int y)> & callback);
	bool iterate(int xmin, int ymin, int xmax, int ymax, bool(*callback)(uint8_t *pixel, int ch, int x, int y, void *args), void *args = nullptr);
	STBImage & set(const STBImage & img, int xmin, int ymin, int xmax, int ymax, int tx, int ty);
	STBImage & set(const RGBATuple & color, int xmin, int ymin, int xmax, int ymax);
	STBImage get(int xmin, int ymin, int xmax, int ymax);
	STBImage scale(int nw, int nh, const STBImage::ScaleOpts filter, const STBImage::ScaleOpts edge, int flags);
	STBImage scale(int xmin, int ymin, int xmax, int ymax, int nw, int nh, const STBImage::ScaleOpts filter, const STBImage::ScaleOpts edge, int flags);
	bool condition(int xmin, int ymin, int xmax, int ymax);

private:

public:
	static bool unsafe_c_iterator_build(unsafe_iterator & it, const STBImage & img, int x, int y, int w, int h);
	static bool unsafe_c_iterator_next(unsafe_iterator & it);
	static bool unsafe_c_iterator2_build(unsafe_iterator & it1, const STBImage & img1, int x1, int y1, unsafe_iterator & it2, const STBImage & img2, int x2, int y2, int w, int h);
	static bool unsafe_c_iterator2_next(unsafe_iterator & it1, unsafe_iterator & it2);
	static uint16_t cover_rgba(STBImage & dst, int xr, int yr, STBImage & back, int xb, int yb, STBImage & front, int xf, int yf, int w, int h, RGBTuple transparent = default_transparent_color, size_t & transparent_count = default_transparent_count);
};

#endif // !STBIMAGE_H__



