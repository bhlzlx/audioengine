#include "owcmn.h"
#include <string.h>

void strrep(char * _szStr, char * _szKeys, char ch)
{
	int nKey = strlen(_szKeys);
	unsigned i;
	for(i = 0;i<strlen(_szStr); ++i)
	{
		for(int j = 0;j<nKey;++j)
		{
			if(_szStr[i] == _szKeys[j])
			{
				_szStr[i] = ch;
			}
		}
	}
}
/*
owINT32 stricmp( const char * _szStr1, const char * _szStr2)
{
	owINT32 len = strlen(_szStr1);
	if(len != strlen(_szStr2) )
	{
		return 1;
	}
	for(int i = 0;i<len;++i)
	{
		char a1 ;
		if(_szStr1[i]>='a' && _szStr1[i]<='z')
		{
			a1 = _szStr1[i] - ('a'-'A');
		}
		else
		{
			a1 = _szStr1[i];
		}
		
		char a2 ;
		if(_szStr2[i]>='a' && _szStr2[i]<='z')
		{
			a2 = _szStr2[i] - ('a'-'A');
		}
		else
		{
			a2 = _szStr2[i];
		}
		if(a1 != a2)
		{
			return 1;
		}
	}
	return 0;
}*/