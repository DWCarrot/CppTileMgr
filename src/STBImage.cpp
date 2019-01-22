#include "pch.h"
#include "STBImage.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif // !STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif // !STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

#ifndef STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#endif // !STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define STBIMG_MALLOC(sz) STBI_MALLOC(sz)
#define STBIMG_REALLOC(p, sz) STBI_REALLOC(p, sz)
#define STBIMG_FREE(p) STBI_FREE(p);

stbir_edge _enum_getEdge(STBImage::ScaleOpts opt)
{
	return (stbir_edge)(opt - STBImage::ScaleOpts::EDGE_CLAMP + stbir_edge::STBIR_EDGE_CLAMP);
}

stbir_filter _enum_getFilter(STBImage::ScaleOpts opt)
{
	return (stbir_filter)(opt - STBImage::ScaleOpts::FILTER_DEFAULT + stbir_filter::STBIR_FILTER_DEFAULT);
}

// 
#ifdef STBIR_FLAG_ALPHA_PREMULTIPLIED
const int STBImage::ResizeFlag_ALPHA_PREMULTIPLIED = STBIR_FLAG_ALPHA_PREMULTIPLIED;
#endif

#ifdef STBIR_FLAG_GAMMA_CORRECT
const int img::ResizeFlag_GAMMA_CORRECT = STBIR_FLAG_GAMMA_CORRECT;
#endif


STBImage::STBImage()
{
	w = 0;
	h = 0;
	ch = 0;
	data = nullptr;
	exception = 0;
}

STBImage::STBImage(int w, int h, int ch)
{
	STBImage();
	int sz = w * h * ch;
	this->data = (uint8_t*)STBIMG_MALLOC(sz);
	if (this->data == nullptr)
	{
		w = 0;
		h = 0;
		ch = 0;
		data = nullptr;
		setException(STBImage::EXCEPTION_MEMORY);
		return;
	}
	memset(this->data, 0, sz);
	this->w = w;
	this->h = h;
	this->ch = ch;
	return;
}

STBImage::STBImage(const STBImage & img)
{
	STBImage();
	if (img.data != nullptr)
	{
		int sz = img.w * img.h * img.ch;
		data = (uint8_t*)STBIMG_MALLOC(sz);
		if (data != nullptr)
		{
			w = img.w;
			h = img.h;
			ch = img.ch;
			memcpy(data, img.data, sz);
			exception = img.exception;
		}
		else
		{
			setException(STBImage::EXCEPTION_MEMORY);
		}
	}
}

STBImage::STBImage(STBImage && img)
{
	STBImage();
	if (img.data != nullptr)
	{
		w = img.w;
		h = img.h;
		ch = img.ch;
		data = img.data;
		exception = img.exception;
	}
	img.w = 0;
	img.h = 0;
	img.ch = 0;
	img.data = nullptr;
	img.exception = 0;
}

STBImage & STBImage::operator=(const STBImage & img)
{
	clear();
	if (img.data != nullptr)
	{
		int sz = img.w * img.h * img.ch;
		data = (uint8_t*)STBIMG_MALLOC(sz);
		if (data != nullptr)
		{
			w = img.w;
			h = img.h;
			ch = img.ch;
			memcpy(data, img.data, sz);
			exception = 0;
		}
		else
		{
			setException(STBImage::EXCEPTION_MEMORY);
		}
	}
	return *this;
}

STBImage & STBImage::operator=(STBImage && img)
{
	clear();
	if (img.data != nullptr)
	{
		w = img.w;
		h = img.h;
		ch = img.ch;
		data = img.data;
		exception = img.exception;
	}
	img.w = 0;
	img.h = 0;
	img.ch = 0;
	img.data = nullptr;
	img.exception = 0;
	return *this;
}

STBImage::~STBImage()
{
	clear();
}

bool STBImage::allocate(int w, int h, int ch)
{
	int sz = w * h * ch;
	if (this->data != nullptr)
	{
		if (sz != this->w * this->h * this->ch)
		{
			STBIMG_FREE(this->data);
			this->data = (uint8_t*)STBIMG_MALLOC(sz);
		}
	}
	else
	{
		this->data = (uint8_t*)STBIMG_MALLOC(sz);
	}
	if (this->data == nullptr)
	{
		w = 0;
		h = 0;
		ch = 0;
		data = nullptr;
		setException(STBImage::EXCEPTION_MEMORY);
		return false;
	}
	memset(this->data, 0, sz);
	this->w = w;
	this->h = h;
	this->ch = ch;
	return true;
}

void STBImage::clear()
{

	if (data != nullptr)
	{
		STBIMG_FREE(data);
		data = nullptr;	//strange compiler problem: (out)
	}
	//data = nullptr;
	w = 0;
	h = 0;
	ch = 0;
	exception = 0;
}

void STBImage::move(STBImage & img)
{
	if (data != nullptr)
	{
		STBIMG_FREE(data);
		data = nullptr;
	}
	if (img.data != nullptr)
	{
		w = img.w;
		h = img.h;
		ch = img.ch;
		data = img.data;
		exception = img.exception;
	}
	else
	{
		w = 0;
		h = 0;
		ch = 0;
		data = nullptr;
		exception = 0;
	}
	img.w = 0;
	img.h = 0;
	img.ch = 0;
	img.data = nullptr;
	img.exception = 0;
}


bool STBImage::iterate(int xmin, int ymin, int xmax, int ymax, bool(*callback)(uint8_t *pixel, int ch, int x, int y, void *args), void *args)
{
	if (data == nullptr)
	{		
		setException(STBImage::EXCEPTION_EMPTY);
		return false;
	}	
	if (xmin < 0 || ymin < 0 || xmax > w || ymax > h || xmin > xmax || ymin > ymax)
	{		
		setException(STBImage::EXCEPTION_SIZE);
		return false;
	}
	int i, j;
	int pixelWB = ch;
	int lineWB = pixelWB * w;
	uint8_t *pC, *pL;
	for (j = ymin, pL = data + (ymin * lineWB); j < ymax; j += 1, pL += lineWB)
	{
		for (i = xmin, pC = pL + (xmin * pixelWB); i < xmax; i += 1, pC += pixelWB)
		{
			if (!(*callback)(pC, pixelWB, i, j, args))
				return false;
		}
	}
	return true;
}

STBImage & STBImage::set(const STBImage & img, int xmin, int ymin, int xmax, int ymax, int tx, int ty)
{
	if (data == nullptr || img.data == nullptr || ch != img.ch)
	{
		setException(STBImage::EXCEPTION_EMPTY);
		return *this;
	}
	if (xmin < 0 || ymin < 0 || xmax > img.w || ymax > img.h || xmax - xmin < 0 || ymax - ymin < 0)
	{
		setException(STBImage::EXCEPTION_SIZE);
		return *this;
	}
	if (tx < 0 || ty < 0 || tx + xmax - xmin > w || ty + ymax - ymin > h)
	{
		setException(STBImage::EXCEPTION_SIZE);
		return *this;
	}

	/*
	int pixelWB = ch;
	int lineWBS = pixelWB * img.w;
	int lineWBD = pixelWB * w;
	int i, j;
	uint8_t *pLS, *pLD, *pCS, *pCD;
	for (j = ymin, pLS = img.data + (ymin * lineWBS), pLD = data + (ty * lineWBD); j < ymax; j += 1, pLS += lineWBS, pLD += lineWBD)
	{
		for (i = xmin, pCS = pLS + (xmin * pixelWB), pCD = pLD + (tx * pixelWB); i < xmax; i += 1, pCS += pixelWB, pCD += pixelWB)
		{

		}
	}
	*/

	int pixelWB = ch;
	int lineWBS = pixelWB * img.w;
	int lineWBD = pixelWB * w;
	int j, sz = pixelWB * (xmax - xmin);
	uint8_t *pLS, *pLD;
	for (j = ymin, pLS = img.data + (ymin * lineWBS + xmin * pixelWB), pLD = data + (ty * lineWBD + tx * pixelWB); j < ymax; j += 1, pLS += lineWBS, pLD += lineWBD)
		memcpy(pLD, pLS, sz);

	return *this;
}

STBImage & STBImage::set(const RGBATuple & color, int xmin, int ymin, int xmax, int ymax)
{
	if (data == nullptr)
	{
		setException(STBImage::EXCEPTION_MEMORY);
		return *this;
	}
	if (xmin < 0 || ymin < 0 || xmax > w || ymax > h || xmax - xmin < 0 || ymax - ymin < 0)
	{
		setException(STBImage::EXCEPTION_SIZE);
		return *this;
	}
	int i, j;
	int pixelWB = ch;
	int lineWB = pixelWB * w;
	uint8_t *pC, *pL;
	uint8_t r = color.r, g = color.g, b = color.b, a = color.a;
	if (ch == 4)
	{
		for (j = ymin, pL = data + (ymin * lineWB); j < ymax; j += 1, pL += lineWB)
		{
			for (i = xmin, pC = pL + (xmin * pixelWB); i < xmax; i += 1, pC += pixelWB)
			{
				pC[0] = r;
				pC[1] = g;
				pC[2] = b;
				pC[3] = a;
			}
		}
		return *this;
	}
	if (ch == 3)
	{
		for (j = ymin, pL = data + (ymin * lineWB); j < ymax; j += 1, pL += lineWB)
		{
			for (i = xmin, pC = pL + (xmin * pixelWB); i < xmax; i += 1, pC += pixelWB)
			{
				pC[0] = r;
				pC[1] = g;
				pC[2] = b;
			}
		}
		return *this;
	}
	if (ch == 2)
	{
		for (j = ymin, pL = data + (ymin * lineWB); j < ymax; j += 1, pL += lineWB)
		{
			for (i = xmin, pC = pL + (xmin * pixelWB); i < xmax; i += 1, pC += pixelWB)
			{
				pC[0] = r;
				pC[1] = a;
			}
		}
		return *this;
	}
	if (ch == 1)
	{
		for (j = ymin, pL = data + (ymin * lineWB); j < ymax; j += 1, pL += lineWB)
		{
			for (i = xmin, pC = pL + (xmin * pixelWB); i < xmax; i += 1, pC += pixelWB)
			{
				pC[0] = r;
			}
		}
		return *this;
	}
	setException(STBImage::EXCEPTION_CHANNEL);
	return *this;
}

STBImage STBImage::get(int xmin, int ymin, int xmax, int ymax)
{
	STBImage res;
	if (data == nullptr)
	{
		res.setException(STBImage::EXCEPTION_EMPTY);
		return res;
	}
	if (xmin < 0 || ymin < 0 || xmax > w || ymax > h || xmin >= xmax || ymin >= ymax)
	{
		res.setException(STBImage::EXCEPTION_SIZE);
		return res;
	}
	if (!res.allocate(xmax - xmin, ymax - ymin, ch))
		return res;
	int j;
	int pixelWB = ch;
	int lineWB = pixelWB * w;
	int lineWBD = pixelWB * res.w;
	uint8_t *pLD, *pL;
	for (j = ymin, pL = data + (ymin * lineWB + xmin * pixelWB), pLD = res.data; j < ymax; j += 1, pL += lineWB, pLD += lineWBD)
	{
		memcpy(pLD, pL, lineWBD);
	}
	return res;
}

STBImage STBImage::scale(int nw, int nh, const STBImage::ScaleOpts filter, const STBImage::ScaleOpts edge, int flags)
{
	STBImage res;
	if (data == nullptr)
	{
		res.setException(STBImage::EXCEPTION_EMPTY);
		return res;
	}
	if (nw <= 0 || nh <= 0 || ch < 0 || ch > 4)
	{
		res.setException(STBImage::EXCEPTION_SIZE);
		return res;
	}
	if (!res.allocate(nw, nh, ch))
		return res;
	int pixelBW = ch;

	int alpha_channel = STBIR_ALPHA_CHANNEL_NONE;

	if (ch == 4 || ch == 2)
		alpha_channel = ch - 1;

	int rt = stbir_resize_region(
		data, w, h, w * pixelBW,
		res.data, nw, nh, nw * pixelBW,
		stbir_datatype::STBIR_TYPE_UINT8, ch, alpha_channel, flags,
		_enum_getEdge(edge), _enum_getEdge(edge),
		_enum_getFilter(filter), _enum_getFilter(filter),
		stbir_colorspace::STBIR_COLORSPACE_LINEAR,
		NULL, 0, 0, 1, 1
	);
	if (!rt)
	{
		res.clear();
		res.setException(STBImage::EXCEPTION_C_RESIZE_FAIL);
	}		
	return res;
}

STBImage STBImage::scale(int xmin, int ymin, int xmax, int ymax, int nw, int nh, const STBImage::ScaleOpts filter, const STBImage::ScaleOpts edge, int flags)
{
	STBImage res;
	if (data == nullptr)
	{
		res.setException(STBImage::EXCEPTION_EMPTY);
		return res;
	}
	if (nw <= 0 || nh <= 0)
	{
		res.setException(STBImage::EXCEPTION_SIZE);
		return res;
	}
	if (xmin < 0 || ymin < 0 || xmax > w || ymax > h || xmin >= xmax || ymin >= ymax)
	{
		res.setException(STBImage::EXCEPTION_SIZE);
		return res;
	}
	if (!res.allocate(nw, nh, ch))
		return res;
	int pixelBW = ch;

	int alpha_channel = STBIR_ALPHA_CHANNEL_NONE;
	if (ch == 4 || ch == 2)
		alpha_channel = ch - 1;

	int rt = stbir_resize_region(
		data + (ymin * w + xmin) * pixelBW, xmax - xmin, ymax - ymin, w * pixelBW,
		res.data, nw, nh, nw * pixelBW,
		stbir_datatype::STBIR_TYPE_UINT8, ch, alpha_channel, flags,
		_enum_getEdge(edge), _enum_getEdge(edge),
		_enum_getFilter(filter), _enum_getFilter(filter),
		stbir_colorspace::STBIR_COLORSPACE_LINEAR,
		NULL, 0, 0, 1, 1
	);
	if (!rt)
	{
		res.clear();
		res.setException(STBImage::EXCEPTION_C_RESIZE_FAIL);
	}
	return res;
}

STBImage & STBImage::load(const std::string & filename)
{
	clear();
	data = stbi_load(filename.c_str(), &w, &h, &ch, 0);
	if(data == nullptr)
	{
		clear();
		setException(STBImage::EXCEPTION_C_LOAD_FAIL);
	}
	return *this;
}


STBImage & STBImage::save(const std::string & filename, int argc, int *argv)
{
	if (data == nullptr)
	{
		setException(STBImage::EXCEPTION_EMPTY);
		return *this;
	}
	char suf[10] = { '\0' };
	int i = filename.rfind('.');
	if (i >= 0)
#ifdef STBI_MSC_SECURE_CRT
		strncpy_s(suf, &filename[i + 1], 9);
#else
		strncpy(suf, &filename[i + 1], 9);
#endif // STBI_MSC_SECURE_CRT	
	ImageFmt fmt = ImageFmt::INVALID;
	if (argc > 0)
		fmt = (ImageFmt)argv[0];
	while(fmt == ImageFmt::INVALID)
	{
		if (strcmp(suf, "png") == 0)
		{
			fmt = ImageFmt::PNG;
			break;
		}
		if (strcmp(suf, "bmp") == 0)
		{
			fmt = ImageFmt::BMP;
			break;
		}
		if (strcmp(suf, "jpg") == 0)
		{
			fmt = ImageFmt::JPEG;
			break;
		}
		break;
	}

	int args[10];
	bool res = false;
	switch (fmt)
	{
	case STBImage::BMP:
		res = stbi_write_bmp(filename.c_str(), w, h, ch, data);
		if(!res)
			setException(STBImage::EXCEPTION_C_SAVE_FAIL);
		return *this;
	case STBImage::JPEG:
		args[0] = 80;
		if (argc > 1)
			args[0] = argv[1];
		res = stbi_write_jpg(filename.c_str(), w, h, ch, data, args[0]);
		if (!res)
			setException(STBImage::EXCEPTION_C_SAVE_FAIL);
		return *this;
	case STBImage::PNG:
		args[0] = stbi_write_png_compression_level;
		args[1] = stbi_write_force_png_filter;
		if (argc > 1 && argv[1] > 0)
			stbi_write_png_compression_level = argv[1];
		if (argc > 2 && argv[2] > 0)
			stbi_write_force_png_filter = argv[2];
		res = stbi_write_png(filename.c_str(), w, h, ch, data, w * ch);
		stbi_write_png_compression_level = args[0];
		stbi_write_force_png_filter = args[1];
		if (!res)
			setException(STBImage::EXCEPTION_C_SAVE_FAIL);
		return *this;
	default:
		setException(STBImage::EXCEPTION_C_SAVE_FAIL);
		return *this;
	}
}

STBImage STBImage::appendAlpha(uint8_t alpha)
{
	STBImage res;
	if (data == nullptr)
	{
		res.setException(STBImage::EXCEPTION_EMPTY);
		return res;
	}
	if (ch == 2 || ch == 4)
	{
		res.setException(STBImage::EXCEPTION_CHANNEL);
		return res;
	}
	const int picSize = w * h;
	if (ch == 3)
	{
		if (!res.allocate(w, h, 4))
			return res;
		for (auto pTgt = res.data, pSrc = data, end = pSrc + (picSize * 3); pSrc < end; pSrc += 3, pTgt += 4)
		{
			pTgt[0] = pSrc[0];
			pTgt[1] = pSrc[1];
			pTgt[2] = pSrc[2];
			pTgt[3] = alpha;
		}
		return res;
	}
	if (ch == 1)
	{
		if (!res.allocate(w, h, 2))
			return res;
		for (auto pTgt = res.data, pSrc = data, end = pSrc + (picSize * 1); pSrc < end; pSrc += 1, pTgt += 2)
		{
			pTgt[0] = pSrc[0];
			pTgt[1] = alpha;
		}
		return res;
	}
	res.setException(STBImage::EXCEPTION_CHANNEL);
	return res;
}

STBImage STBImage::appendAlpha(const RGBTuple & transparent)
{
	STBImage res;
	if (data == nullptr)
	{
		res.setException(STBImage::EXCEPTION_EMPTY);
		return res;
	}
	if(ch == 2 || ch == 4)
	{
		res.setException(STBImage::EXCEPTION_CHANNEL);
		return res;
	}
	const int picSize = w * h;
	uint8_t tr = transparent.r, tg = transparent.g, tb = transparent.b;
	if (ch == 3)
	{
		if (!res.allocate(w, h, 4))
			return res;
		bool er, eg, eb;
		for (auto pTgt = res.data, pSrc = data, end = pSrc + (picSize * 3); pSrc < end; pSrc += 3, pTgt += 4)
		{
			er = (pTgt[0] = pSrc[0]) == tr;
			eg = (pTgt[1] = pSrc[1]) == tg;
			eb = (pTgt[2] = pSrc[2]) == tb;
			if (er && eg && eb)
				pTgt[3] = 0x00;
			else
				pTgt[3] = 0xFF;
		}
		return res;
	}
	if (ch == 1)
	{
		if (!res.allocate(w, h, 2))
			return res;
		for (auto pTgt = res.data, pSrc = data, end = pSrc + (picSize * 1); pSrc < end; pSrc += 1, pTgt += 2)
		{
			if ((pTgt[0] = pSrc[0]) == tr)
				pTgt[1] = 0x00;
			else
				pTgt[1] = 0xFF;
		}
		return res;
	}
	res.setException(STBImage::EXCEPTION_CHANNEL);
	return res;
}

STBImage STBImage::removeAlpha()
{
	STBImage res;
	if (data == nullptr)
	{
		res.setException(STBImage::EXCEPTION_EMPTY);
		return res;
	}
	if (ch == 1 || ch == 3)
	{
		res.setException(STBImage::EXCEPTION_CHANNEL);
		return res;
	}
	const int picSize = w * h;
	if (ch == 4)
	{
		if (!res.allocate(w, h, 3))
			return res;
		for (auto pTgt = res.data, pSrc = data, end = pSrc + (picSize * 4); pSrc < end; pSrc += 4, pTgt += 3)
		{
			pTgt[0] = pSrc[0];
			pTgt[1] = pSrc[1];
			pTgt[2] = pSrc[2];
		}
		return res;
	}
	if (ch == 2)
	{
		if (!res.allocate(w, h, 1))
			return res;
		for (auto pTgt = res.data, pSrc = data, end = pSrc + (picSize * 2); pSrc < end; pSrc += 2, pTgt += 1)
		{
			pTgt[0] = pSrc[0];
		}
		return res;
	}
	res.setException(STBImage::EXCEPTION_CHANNEL);
	return res;
}

STBImage STBImage::removeAlpha(const RGBTuple & transparent)
{
	STBImage res;
	if (data == nullptr)
	{
		res.setException(STBImage::EXCEPTION_EMPTY);
		return res;
	}
	if (ch == 1 || ch == 3)
	{
		res.setException(STBImage::EXCEPTION_CHANNEL);
		return res;
	}
	const int picSize = w * h;
	uint8_t tr = transparent.r, tg = transparent.g, tb = transparent.b;
	if (ch == 4)
	{
		if (!res.allocate(w, h, 3))
			return res;
		for (auto pTgt = res.data, pSrc = data, end = pSrc + (picSize * 4); pSrc < end; pSrc += 4, pTgt += 3)
		{
			if (pSrc[3] == 0x00)
			{
				pTgt[0] = tr;
				pTgt[1] = tg;
				pTgt[2] = tb;
			}
			else
			{
				pTgt[0] = pSrc[0];
				pTgt[1] = pSrc[1];
				pTgt[2] = pSrc[2];
			}
		}
		return res;
	}
	if (ch == 2)
	{
		if (!res.allocate(w, h, 1))
			return res;
		for (auto pTgt = res.data, pSrc = data, end = pSrc + (picSize * 2); pSrc < end; pSrc += 2, pTgt += 1)
		{
			if (pSrc[1] == 0x00)
			{
				pTgt[0] = tr;
			}
			else
			{
				pTgt[0] = pSrc[0];
			}
		}
		return res;
	}
	res.setException(STBImage::EXCEPTION_CHANNEL);
	return res;
}

bool STBImage::hasColor(const RGBTuple & color)
{
	if (data == nullptr)
		return false;
	if (ch < 3)
		return false;
	const int bw = ch;
	for (uint8_t *p = data, *end = data + (w * h * bw); p < end; p += bw)
	{
		if (p[0] == color.r && p[1] == color.g && p[2] == color.b)
			return true;
	}
	return false;
}

bool STBImage::hasColor(const RGBATuple & color)
{
	if (data == nullptr)
		return false;
	if (ch < 4)
		return false;
	const int bw = ch;
	for (uint8_t *p = data, *end = data + (w * h * bw); p < end; p += bw)
	{
		if (p[0] == color.r && p[1] == color.g && p[2] == color.b && p[3] == color.a)
			return true;
	}
	return false;
}

bool STBImage::allColor(const RGBTuple & color)
{
	if (data == nullptr)
		return false;
	if (ch < 3)
		return false;
	const int bw = ch;
	for (uint8_t *p = data, *end = data + (w * h * bw); p < end; p += bw)
	{
		if (p[0] != color.r || p[1] != color.g || p[2] != color.b)
			return false;
	}
	return true;
}

bool STBImage::allColor(const RGBATuple & color)
{
	if (data == nullptr)
		return false;
	if (ch < 4)
		return false;
	const int bw = ch;
	for (uint8_t *p = data, *end = data + (w * h * bw); p < end; p += bw)
	{
		if (p[0] != color.r || p[1] != color.g || p[2] != color.b || p[3] != color.a)
			return false;
	}
	return true;
}

bool STBImage::iterate(int xmin, int ymin, int xmax, int ymax, const std::function<bool(uint8_t*pixel, int ch, int x, int y)>& callback)
{
	if (data == nullptr)
	{
		setException(STBImage::EXCEPTION_EMPTY);
		return false;
	}
	if (xmin < 0 || ymin < 0 || xmax > w || ymax > h || xmin >= xmax || ymin >= ymax)
	{
		setException(STBImage::EXCEPTION_SIZE);
		return false;
	}
	int i, j;
	int pixelWB = ch;
	int lineWB = pixelWB * w;
	uint8_t *pC, *pL;
	for (j = ymin, pL = data + (ymin * lineWB); j < ymax; j += 1, pL += lineWB)
	{
		for (i = xmin, pC = pL + (xmin * pixelWB); i < xmax; i += 1, pC += pixelWB)
		{
			if (!callback(pC, pixelWB, i, j))
				return false;
		}
	}
	return true;
}


bool STBImage::unsafe_c_iterator_build(unsafe_iterator & it, const STBImage & img, int x, int y, int w, int h)
{
	w = x + w;//xmax
	h = y + h;//ymax
	if (x < 0 || y < 0 || w > img.w || h > img.h || x >= w || y >= h)
	{
		memset(&it, 0, sizeof(it));
		return false;
	}
	it.ch = img.ch;
	it.xmin = x;
	it.xmax = w;
	it.ymin = y;
	it.ymax = h;
	it.i = x;
	it.j = y;
	it._lineWB = it.ch * img.w;
	it._pL = img.data + (y * img.w + x) * it.ch;
	it._pC = it._pL;
	it.next = nullptr;
	return true;
}

bool STBImage::unsafe_c_iterator_next(unsafe_iterator & it)
{
	if (it._pC == nullptr)
	{
		return false;
	}
	if (it.i < it.xmax)
	{
		it.next = it._pC;
		it.i += 1;
		it._pC += it.ch;
		return true;
	}
	it.j += 1;
	it._pL += it._lineWB;
	if (it.j < it.ymax)
	{
		it.i = it.xmin;
		it._pC = it._pL;
		it.next = it._pL;
		it.i += 1;
		it._pC += it.ch;
		return true;
	}
	memset(&it, 0, sizeof(unsafe_iterator));
	return false;
}


bool STBImage::unsafe_c_iterator2_build(unsafe_iterator & it1, const STBImage & img1, int x1, int y1, unsafe_iterator & it2, const STBImage & img2, int x2, int y2, int w, int h)
{
	if (
		x1 < 0 || y1 < 0 || x1 + w > img1.w || y1 + h > img1.h
		|| x2 < 0 || y2 < 0 || x2 + w > img2.w || y2 + h > img2.h
		|| w <= 0 || h <= 0
		)
	{
		memset(&it1, 0, sizeof(it1));
		memset(&it2, 0, sizeof(it2));
		return false;
	}
	it1.ch = img1.ch;
	it1.xmin = x1;
	it1.xmax = x1 + w;
	it1.ymin = y1;
	it1.ymax = y1 + h;
	it1.i = x1;
	it1.j = y1;
	it1._lineWB = it1.ch * img1.w;
	it1._pL = img1.data + (y1 * img1.w + x1) * it1.ch;
	it1._pC = it1._pL;
	it1.next = nullptr;
	it2.ch = img2.ch;
	it2.xmin = x2;
	it2.xmax = x2 + w;
	it2.ymin = y2;
	it2.ymax = y2 + h;
	it2.i = x2;
	it2.j = y2;
	it2._lineWB = it2.ch * img2.w;
	it2._pL = img2.data + (y2 * img2.w + x2) * it2.ch;
	it2._pC = it2._pL;
	it2.next = nullptr;
	return true;
}

bool STBImage::unsafe_c_iterator2_next(unsafe_iterator & it1, unsafe_iterator & it2)
{
	if (it1._pL == nullptr || it2._pL == nullptr)
	{
		return false;
	}
	if (it1.i < it1.xmax)
	{
		it1.next = it1._pC;
		it2.next = it2._pC;
		it1.i += 1;
		it2.i += 1;
		it1._pC += it1.ch;
		it2._pC += it2.ch;
		return true;
	}
	it1.j += 1;
	it2.j += 1;
	it1._pL += it1._lineWB;
	it2._pL += it2._lineWB;
	if (it1.j < it1.ymax)
	{
		it1.i = it1.xmin;
		it2.i = it2.xmin;
		it1._pC = it1._pL;
		it2._pC = it2._pL;
		it1.next = it1._pL;
		it2.next = it2._pL;
		it1.i += 1;
		it2.i += 1;
		it1._pC += it1.ch;
		it2._pC += it2.ch;
		return true;
	}
	memset(&it1, 0, sizeof(it1));
	return false;
}

uint16_t STBImage::cover_rgba(STBImage & dst, int xr, int yr, STBImage & back, int xb, int yb, STBImage & front, int xf, int yf, int w, int h, RGBTuple transparent, size_t & transparent_count)
{
	if (dst.data == nullptr || back.data == nullptr || front.data == nullptr)
		return STBImage::EXCEPTION_EMPTY;
	if (!(dst._checkboard(xr, yr, xr + w, yr + h) && back._checkboard(xb, yb, xb + w, yb + h) && front._checkboard(xf, yf, xf + w, yf + h)))
		return STBImage::EXCEPTION_SIZE;
	if (!(back.getC() >= 3 && front.getC() >= 3 && dst.getC() >= back.getC() && dst.getC() >= front.getC()))
		return STBImage::EXCEPTION_CHANNEL;

	if ((dst.data == back.data && yr * dst.w + xr > yb * back.w + xb) || (dst.data == front.data && yr * dst.w + xr > yf * front.w + xf))
	{

	}

	int i, j;
	const int bw = dst.getC(), bw1 = front.getC(), bw2 = back.getC();
	const bool alpha = (bw == 4), alpha1 = (bw1 == 4), alpha2 = (bw2 == 4);
	const int lw = dst.w * bw, lw1 = front.w * bw1, lw2 = back.w * bw2;
	uint8_t *p, *p1, *p2, *pL, *p1L, *p2L;
	uint32_t r, g, b, a, r1, g1, b1, a1, r2, g2, b2, a2, tmp1, tmp2;
	const uint32_t tr = transparent.r, tg = transparent.g, tb = transparent.b;

	transparent_count = 0;

	for (
		j = 0, pL = dst.data + (yr * dst.w + xr) * bw, p1L = front.data + (yf * front.w + xf) * bw1, p2L = back.data + (yb * front.w + xb) * bw2;
		j < h;
		j += 1, pL += lw, p1L += lw1, p2L += lw2
		) {

		for (i = 0, p = pL, p1 = p1L, p2 = p2L;
			i < w;
			i += 1, p += bw, p1 += bw1, p2 += bw2
			) {

			r1 = p1[0];
			g1 = p1[1];
			b1 = p1[2];
			a1 = alpha1 ? p1[3] : ((r1 != tr) || (g1 != tg) || (b1 != tb)) * 0xFF;

			r2 = p2[0];
			g2 = p2[1];
			b2 = p2[2];
			a2 = alpha2 ? p2[3] : ((r2 != tr) || (g2 != tg) || (b2 != tb)) * 0xFF;

			tmp1 = 0xFF * a1;
			tmp2 = a2 * (0xFF - a1);
			a = (0xFF * 0xFF) - (0xFF - a2) * (0xFF - a1);

			if (a > 0)
			{
				r = (r1 * tmp1 + r2 * tmp2) / a;
				g = (g1 * tmp1 + g2 * tmp2) / a;
				b = (b1 * tmp1 + b2 * tmp2) / a;
				a = a / 255;
			}
			else
			{
				r = tr;
				g = tg;
				b = tb;
				a = 0;
			}

			p[0] = r;
			p[1] = g;
			p[2] = b;
			if (a < 0xFF)
				transparent_count++;
			if (alpha)
				p[3] = a;

		}
	}

	return 0;
}











