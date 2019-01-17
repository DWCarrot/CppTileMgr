// Ctilemgr.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <string>
#include <string.h>
#include <exception>

#include "STBImage.h"
#include "Util.h"
#include "TileManager.h"

RGBATuple c = { 0,0,0,0 };
uint8_t *cl = (uint8_t*)&c;

#define B1

bool setColor(uint8_t* pixel, int bw, int i, int j, void *args)
{
	return memcpy(pixel, args, bw);
}

size_t func_ex(std::string & s)
{
	if (s.length() < 3)
		throw std::invalid_argument("length < 3");
	return s.find('.');
}


void test1()
{
	std::string s;
	STBImage t, m;

	Timer timer = Timer();


	std::function<bool(uint8_t *pixel, int ch, int x, int y)> f = [](uint8_t* pixel, int bw, int i, int j) -> bool {return memcpy(pixel, cl, bw); };



	

#ifdef B1

	std::cout << "iter,pointer\n\r";
	t.allocate(256, 256, 4);
	for (int i = 0; i < 100; ++i)
	{
		timer.start();
		c = { 192, 168,137,255 };
		t.iterate(0, 0, 128, 192, setColor, cl);
		c = { 180, 243, 98, 65 };
		t.iterate(82, 93, 240, 100, setColor, cl);
		timer.stop();

		printf("  %6d", (int)(timer.get() * 1000 * 1000));
	}
	t.save("t.png");
	std::cout << "\n\r..end" << std::endl;

	std::cout << "iter,std::function\n\r";
	t.allocate(256, 256, 4);
	for (int i = 0; i < 100; ++i)
	{
		timer.start();
		c = { 192, 168,137,255 };
		t.iterate(0, 0, 128, 192, f);
		c = { 180, 243, 98, 65 };
		t.iterate(82, 93, 240, 100, f);
		timer.stop();

		printf("  %6d", (int)(timer.get() * 1000 * 1000));
	}
	t.save("t.png");
	std::cout << "\n\r..end" << std::endl;

	std::cout << "inline\n\r";
	t.allocate(256, 256, 4);
	for (int i = 0; i < 100; ++i)
	{
		timer.start();
		c = { 192, 168,137,255 };
		t.set(c, 0, 0, 128, 192);
		c = { 180, 243, 98, 65 };
		t.set(c, 82, 93, 240, 100);
		timer.stop();

		printf("  %6d", (int)(timer.get() * 1000 * 1000));
	}
	t.save("t.png");
	std::cout << "\n\r..end" << std::endl;

#endif // B1

	std::cout << "iterator (" << sizeof(unsafe_iterator) << ")\n\r";
	t.allocate(256, 256, 4);
#ifdef B1
	for (int i = 0; i < 100; ++i)
	{
#endif // B1
		unsafe_iterator it;
		timer.start();
		c = { 192, 168,137,255 };
		STBImage::unsafe_c_iterator_build(it, t, 0, 0, 128 - 0, 192 - 0);
		while (STBImage::unsafe_c_iterator_next(it))
			memcpy(it.next, &c, it.ch);
		c = { 180, 243, 98, 65 };
		STBImage::unsafe_c_iterator_build(it, t, 82, 93, 240 - 82, 100 - 93);
		while (STBImage::unsafe_c_iterator_next(it))
			memcpy(it.next, &c, it.ch);
		timer.stop();
		printf("  %6d", (int)(timer.get() * 1000 * 1000));
#ifdef B1
	}
#endif // B1
	int ap[] = { STBImage::ImageFmt::PNG, 2 };
	t.save("t2.png", sizeof(ap) / sizeof(*ap), ap);
	std::cout << "\n\r..end" << std::endl;

	std::cout << "m: ";
	m.allocate(256, 256, 3);
	timer.start();
	c = { 137, 165, 254, 255 };
	m.set(c, 64, 64, 180, 130);
	c = { 0,0,0,0 };
	m.set(c, 80, 90, 110, 120);
	timer.stop();

	printf("  %6d", (int)(timer.get() * 1000 * 1000));

	std::cout <<  std::endl;

	t.save("t.png");
	m.save("m.png");

	STBImage c;
	size_t a;

	std::cout << "updateTile c1: ";
	c = m.appendAlpha({0,0,0});
	
	timer.start();
	STBImage::cover_rgba(c, 0, 0, m, 0, 0, t, 0, 0, c.getW(), c.getH(), { 0,0,0 }, a);
	timer.stop();

	printf("  %6d", (int)(timer.get() * 1000 * 1000));
	std::cout << std::endl;

	c = c.removeAlpha({0,0,0}).scale(c.getW() / 2, c.getH() / 2, STBImage::ScaleOpts::FILTER_DEFAULT, STBImage::ScaleOpts::EDGE_CLAMP, STBImage::ResizeFlag_ALPHA_PREMULTIPLIED);

	printf("===size: %d, %d, (%d)\n\r", c.getW(), c.getH(), c.getC());
	c.save("c1.png");

	std::cout << "updateTile c2: ";
	
	c.allocate(256, 256, 3);
	c = c.appendAlpha({ 0,0,0 });

	timer.start();
	
	STBImage::cover_rgba(c, 0, 0, t, 0, 0, m, 0, 0, c.getW(), c.getH(), { 0,0,0 }, a);

	timer.stop();

	printf("  %6d", (int)(timer.get() * 1000 * 1000));
	std::cout << std::endl;


	timer.start();

	c = c.scale(c.getW() / 2, c.getH() / 2, STBImage::ScaleOpts::FILTER_DEFAULT, STBImage::ScaleOpts::EDGE_CLAMP, STBImage::ResizeFlag_ALPHA_PREMULTIPLIED);

	timer.stop();

	printf("  %6d", (int)(timer.get() * 1000 * 1000));
	std::cout << std::endl;


	printf("===size: %d, %d, (%d)\n\r", c.getW(), c.getH(), c.getC());
	c.save("c2.png");

	std::cout << std::endl;
}

#include<queue>

void test2()
{
	std::string s;
	std::queue<STBImage> q;
	while (true)
	{
		std::cout << "File: ";
		std::getline(std::cin, s);
		if (s.empty())
			break;
		auto t = STBImage();
		t.load(s);
		q.push(t);
		int len = q.size();
		std::cout << "now: " << len << std::endl;
		if (len >= 4)
		{
			if (t.allocate(1024, 1024, 4))
			{

				for(int ct = 0; !q.empty(); ++ct)
				{
					auto f = q.front();
					q.pop();
					if (f.getC() != 4)
						f = f.appendAlpha({ 0,0,0 });
					t.set(f, 0, 0, 512, 512, 512 * (ct & 0x1), 256 * (ct & 0x2));
				}
				t = t.scale(512, 512, STBImage::ScaleOpts::FILTER_DEFAULT, STBImage::ScaleOpts::EDGE_CLAMP, STBImage::ResizeFlag_ALPHA_PREMULTIPLIED);
				t.save("h1.png");
				t = t.removeAlpha({ 0,0,0 });
				t.save("h2.png");
			}
		}
	}
}

void test3()
{
	Timer timer;
	timer.start();
	TileManager mgr(0, 5, 512, 512);
	for (int y = -10; y < 10; ++y)
	{
		for (int x = -10; x < 10; ++x)
		{
			mgr.add(TileId(5, x, y));
		}
	}
	timer.stop();
	printf("  %6d\n", (int)(timer.get() * 1000 * 1000));
	TileId p(0, 0, 0);
	std::function<int(const TileId&, const std::set<TileId>&)> cb = [](TileId i, const std::set<TileId>& list)->int {return 1;  printf("tile (%d,(%d,%d)) \n", i.z, i.x, i.y);};
	for (auto i : mgr.getList())
	{
		if (i.z > mgr.zmin())
			break;
		timer.start();
		mgr.route(i, cb);
		timer.stop();
		printf("(%d,(%d,%d))", i.z, i.x, i.y);
		printf("  %6d\n", (int)(timer.get() * 1000 * 1000));
	}
	
	for (auto i : mgr.getList())
	{
		if (i.z > mgr.zmin())
			break;
		timer.start();
		mgr.update(i);
		timer.stop();
		printf("(%d,(%d,%d))", i.z, i.x, i.y);
		printf("  %6d\n", (int)(timer.get() * 1000 * 1000));
	}

	
}


void test4()
{
	std::cout << Miscellaneous::getCurrentDir() << std::endl;
	
	std::vector<std::string> list = { "test/py2/","test/py1/3/" ,"test/py1/3/5/6/7/8/9/0" };
	for (auto &s : list)
	{
		Miscellaneous::replaceSplit(s);
		int e = Miscellaneous::mkdirs(s);
		if (e != 0)
		{
#ifdef _MSC_VER
			char buf[256];
			strerror_s(buf, 256, e);
			perror(buf);		
#else
			perror(strerror(e));
#endif // _MSC_VER			
		}
		else
		{
			puts(Miscellaneous::absolute(s).c_str());
		}
			
	}



}

void test5()
{
	TileManager mgr(0, 5, 512, 512);
	std::string s;
	
	while (true)
	{
		std::getline(std::cin, s);
		if (s.empty())
			break;
		if (s == "#build")
		{
			for (int i = 0; i <= 5; ++i)
			{
				std::cout << mgr.buildPath(TileId(i, (i * 13) ^ (~0), (i * 37) ^ (-62)), true) << std::endl;
			}
		}
		std::cout << mgr.registerPath(s) << std::endl;
		std::cout << mgr.buildPath(TileId(4, 0, -1)) << std::endl;
	}
}

void test6()
{
	std::string s;
	while (true)
	{
		std::getline(std::cin, s);
		if (s.empty())
			break;
		RGBTuple r1;
		int x;
		int y;
		if (parseRGBTuple(r1, s) == 0)
			printf("RGB: r=%d,g=%d,b=%d\n", r1.r, r1.g, r1.b);
		if (parseRGBTuple2(r1, s) == 0)
			printf("RGB#: r=%d,g=%d,b=%d\n", r1.r, r1.g, r1.b);
		if(parseXY(x, y, s) == 0)
			printf("XY: (%d,%d)\n", x, y);
	}
}

int main(int argc, char * const argv[])
{
    std::cout << "Hello World!\n";

	std::cout << sizeof(STBImage) << std::endl;

	demo(argc, argv);

	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
