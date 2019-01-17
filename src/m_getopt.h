#ifndef M_GETSHORTOPT_H__
#define M_GETSHORTOPT_H__

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
			*opt = '?';
			*optarg = arg;
			break;
		}
	}
	else
	{
		*opt = '\0';
		*optarg = arg;
	}
	return 0;
}

#endif // !M_GETSHORTOPT_H__

