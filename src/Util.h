#ifndef UTIL_TILEMGR_H__
#define UTIL_TILEMGR_H__

#include <map>
#include <stdio.h>  /* defines FILENAME_MAX */
#include <string>

#ifdef _WIN32
#include <direct.h>
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif


class Miscellaneous
{

private:

	inline static int __mkdir(const char *path, int mode)
	{
#ifdef _WIN32
		return _mkdir(path);
#else
		return ::mkdir(path, mode);
#endif
	}

public:
#ifdef _WIN32
	static const char split = '\\';
#else
	static const char split = '/';
#endif

	static std::string getCurrentDir(void)
	{
#ifdef _WIN32
		char *buff = _getcwd(NULL, 0);
#else
		char *buff = getcwd(NULL, 0);
#endif // _WIN32	
		std::string current_working_dir;
		if (buff != nullptr)
		{
			current_working_dir.append(buff);
			free(buff);
		}
		return current_working_dir + split;
	}


	static int mkdir(std::string dir, int mode = 0755)
	{
		if (dir[dir.length() - 1] == split)
			dir.erase(dir.length() - 1);
		errno = 0;
		int stats = __mkdir(dir.c_str(), mode);
		if (stats < 0)
			stats = errno;
		return stats;
	}

	static bool isabsolute(const std::string & path)
	{
#ifdef _WIN32
		int i = path.find('\\');
		return (i > 1 && path[i - 1] == ':');
#else
		return path[0] == '/';
#endif
	}

	static void replaceSplit(std::string & path)
	{
#ifdef _MSC_VER
		char osp = '/', sp = '\\';
#else
		char osp = '\\', sp = '/';
#endif // _MSC_VER
		for (auto p = &path[0], end = &path[path.length() - 1]; p <= end; ++p)
			if (*p == osp)
				*p = sp;
	}

	static std::string absolute(std::string path)
	{
		if (path.empty())
			return std::string();
		if(!isabsolute(path))
			path = getCurrentDir().append(path);
		const int len = path.length();
		char *buf = new char[len + 1];
		char *p = buf;
		for (int i = 0, j = 0; i < len; i = j)
		{
			j = path.find(split, i + 1);
			if (j < 0)
				j = len;
			if (j - i == 1)
			{
				continue;
			}
			if (j - i == 2 && path[i + 1] == '.')
			{
				continue;
			}
			if (j - i == 3 && path[i + 1] == '.' && path[i + 2] == '.')
			{
				for (p = p - 1; p >= buf && *p != split; --p);
				if (p < buf)
					break;
				continue;
			}
			memcpy(p, &path[i], j - i);
			p += (j - i);
		}	
		if (p > buf)
		{
			if (path[len - 1] == split)
				*p++ = split;
			*p = '\0';
			path.clear();
			path.append(buf);
		}
		delete[] buf;
		return path;
	}

	static int mkdirs(std::string dir, int mode = 0755)
	{
		dir = absolute(dir);
#ifdef _WIN32
		const int error_file_not_found = ERROR_FILE_NOT_FOUND;
#else
		const int error_file_not_found = ENOENT;
#endif
		if (dir[dir.length() - 1] == split)
			dir.erase(dir.length() - 1);
		errno = 0;
		__mkdir(dir.c_str(), mode);
		if (errno == error_file_not_found)
		{
			char *buf = new char[dir.size() + 1];
			int i;
			for (i = dir.rfind(split); i > 0; i = dir.rfind(split, i - 1))
			{
				memcpy(buf, dir.c_str(), i);
				buf[i] = '\0';
				errno = 0;
				if (__mkdir(buf, mode) != 0)
				{
					if (errno == error_file_not_found)
					{
						continue;
					}
					i = -1;
					break;
				}
				break;
			}
			if (i > 0)
			{
				const int len = dir.length();
				while(i < len)
				{
					i = dir.find(split, i + 1);
					if (i < 0)
						i = len;
					memcpy(buf, dir.c_str(), i);
					buf[i] = '\0';
					errno = 0;
					if (__mkdir(buf, mode) != 0)
					{
						break;
					}
				}
			}
			delete[] buf;
			return errno;
		}
		return errno;
	}
};

class Memory
{
private:

	size_t memory_usage;

	size_t max_usage;

	std::map<void*, size_t> memory_list;

public:

	Memory()
	{
		memory_usage = 0;
		max_usage = 0;
		memory_list.clear();
	}

	~Memory()
	{
		if (max_usage == 0)
			return;
		printf("\n\r");
		printf(" #Memory max-usage: %zd   | leak :%zd\n\r", max_usage, memory_usage);
		for (auto i : memory_list)
		{
			free(i.first);
			memory_usage -= i.second;
			printf("#Memory release %lX (%zd)\n\r", (uint64_t)i.first, i.second);
		}
		memory_list.clear();
	}

	void * rt_malloc(size_t _Size, const char *__file__ = nullptr, int __line__ = 0)
	{
		void *p = malloc(_Size);
		if (p != nullptr)
		{
			memory_list.emplace(std::make_pair(p, _Size));
			memory_usage += _Size;
			if (max_usage < memory_usage)
				max_usage = memory_usage;
		}		
		return p;
	}

	void *rt_realloc(void *_Block, size_t _Size, const char *__file__ = nullptr, int __line__ = 0)
	{
		if (_Block != nullptr)
		{
			auto i = memory_list.find(_Block);
			if (i != memory_list.end())
			{
				memory_usage -= i->second;
				memory_list.erase(i);
			}
			else
			{
				printf("\n\r>Memory warn[realloc] %lX\n\r", (uint64_t)_Block);
				if (__file__ != nullptr)
					printf("\t@ %s(%d)\n\r", __file__, __line__);
			}
		}
		void *p = realloc(_Block, _Size);
		if (p != nullptr)
		{
			memory_list.emplace(std::make_pair(p, _Size));
			memory_usage += _Size;
			if (max_usage < memory_usage)
				max_usage = memory_usage;
		}
		return p;
	}

	void rt_free(void *_Block, const char *__file__ = nullptr, int __line__ = 0)
	{
		if (_Block != nullptr)
		{
			auto i = memory_list.find(_Block);
			if (i == memory_list.end())
			{
				printf("\n\r>Memory warn[free] %lX\n\r", (uint64_t)_Block);
				if (__file__ != nullptr)
					printf("\t@ %s(%d)\n\r", __file__, __line__);
				return;
			}
			memory_usage -= i->second;
			memory_list.erase(i);
		}
		free(_Block);
	}
};




#ifdef _WIN32



class Timer
{
public:
	Timer() {}
	~Timer() {}
	void start()
	{
		QueryPerformanceFrequency(&nFreq);
		QueryPerformanceCounter(&nBeginTime);
	}
	void stop()
	{
		QueryPerformanceCounter(&nEndTime);
	}
	double get()
	{
		return (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)(nFreq.QuadPart);
	}
private:
	LARGE_INTEGER nFreq;
	LARGE_INTEGER nBeginTime;
	LARGE_INTEGER nEndTime;
};


#else

#include <sys/time.h> 

class Timer
{
public:
	Timer() {}
	~Timer() {}
	void start()
	{
		gettimeofday(&tpstart, NULL);
	}
	void stop()
	{
		gettimeofday(&tpend, NULL);
	}
	double get()
	{
		return (double)(tpend.tv_sec - tpstart.tv_sec) + (double)(tpend.tv_usec - tpstart.tv_usec) / (double)(1000 * 1000);
	}
private:
	struct timeval tpstart;
	struct timeval tpend;
};

#endif 

#endif