/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: avm
 *
 * Created on 20 сентября 2016 г., 22:26
 */


#include <stdio.h>
#include <tchar.h>
#include <string.h>


#define STR_BUFFER_SIZE 1024

char* getProgramParameter(int argc, char* argv[], char* parName)
{
	for (int i = 0; i < argc; i++)
	{
		if (0 == strcmp(argv[i], parName))
		{
			if ((i + 1) < argc)
			{
				return argv[i + 1];
			}
		}
		
	}

	return NULL;
}

char* skipWhiteSpaces(char* src)
{
	char* sWhitespaces = " \t\r\n";
	char* cur = src;
	while (*cur != 0 && strchr(sWhitespaces, *cur))
	{
		cur++;
	}

	if (*cur == 0)
	{
		return NULL;
	}
	else
	{
		return cur;
	}
}


char* transformLineToValue(char* sLine, char* sTokenToLookup)
{
	char* cur = skipWhiteSpaces(sLine);
	if(NULL == cur)
	{
		return NULL;
	}

	if (*cur == ';')
	{
		return NULL;
	}

	int nameLen = strcspn(cur, " \t=");
	if(nameLen == strlen(cur))
	{ 
		return NULL;
	}

	if (0 != strncmp(cur, sTokenToLookup, nameLen))
	{
		return NULL;
	}

	cur = strchr(cur, '=');

	if(NULL == cur)
	{ 
		return NULL;
	}

	cur++;

	if(strlen(cur) == 0)
	{
		strcpy(sLine, cur);
		return cur;
	}

	cur = skipWhiteSpaces(cur);

	char* comment = strchr(cur, ';');

	if(NULL != comment)
	{ 
		*comment = 0;
	}
	else
	{
		comment = cur + strlen(cur);
	}
	

	while (cur != comment)
	{
		comment--;
		if (!strchr(" \t\r\n", *comment))
		{
			break;
		}
	}
	
	if (!strchr(" \t\r\n", *comment))
	{
		comment[1] = 0;
	}
	else
	{
		comment[0] = 0;
	}

	strcpy(sLine, cur);
	return cur;
}

char* getSrcTokenValue(char* sSourceFileName, char* sTokenToLookup, char* sTokenValue)
{
	FILE *fp = fopen(sSourceFileName, "r");
	if (NULL == fp)
	{
		return NULL;
	}
	
	while(fgets(sTokenValue, STR_BUFFER_SIZE, fp) != NULL)
	{ 
		if (NULL != transformLineToValue(sTokenValue,sTokenToLookup))
		{		
			return sTokenValue;
		}
	}

	fclose(fp);

	return NULL;
}


bool findTokenAndInsertVal(char* sLine, char* sTokenToLookup, char* sTokenValue)
{
	char* cur = skipWhiteSpaces(sLine);
	if (NULL == cur)
	{
		return false;
	}

	if (*cur == ';' || strncmp(cur, "//", 2) == 0)
	{
		return false;
	}

	int nameLen = strcspn(cur, " \t=");
	if (nameLen == strlen(cur))
	{
		return false;
	}

	if (0 != strncmp(cur, sTokenToLookup, nameLen))
	{
		return false;
	}

	cur = strchr(cur, '=');

	if (NULL == cur)
	{
		return false;
	}

	cur++;

	char* valbegin = skipWhiteSpaces(cur);
	char* valend = NULL;
	if(NULL == valbegin)
	{
		valbegin = cur;
		valend = cur + strlen(cur);
	}
	else
	{
		char* comm1 = strchr(cur, ';');
		char* comm2 = strstr(cur, "//");
		char* comm = NULL;
		if (NULL == comm1 && NULL == comm2)
		{
			comm = NULL;
		}
		else
		if(comm1 != NULL && comm2 != NULL)
		{ 
			if( (comm1 - cur) < (comm2 - cur))
			{
				comm = comm1;
			}
			else
			{
				comm = comm2;
			}
		}
		else
		if(comm1 != NULL)
		{
			comm = comm1;
		}
		else
		{
			comm = comm2;
		}

		valend = comm;
		if (NULL == valend)
		{
			valend = cur + strlen(cur);
		}
	}

	while (valbegin != valend)
	{
		valend--;
		if (!strchr(" \t\r\n", *valend))
		{
			valend++;
			break;
		}
	}

	if (valend - valbegin > strlen(sTokenValue))
	{
		strcpy(valbegin, sTokenValue);
		strcpy(valbegin + strlen(valbegin), valend);
	}
	else
	{
		char* s = valend;
		char* d = valbegin + strlen(sTokenValue);
		int len = strlen(s)+1;
		s += len-1;
		d += len-1;
		while (len != 0)
		{
			*d = *s;
			d--;
			s--;
			len--;
		}
		memcpy(valbegin, sTokenValue, strlen(sTokenValue));
	}

	return true;

}

#define TEMP_FILE_NAME "__tmp.txt"

bool processResultFile(char* sDestFileName, char* sTokenToLookup, char* sTokenValue)
{
	FILE *fsrc = fopen(sDestFileName, "r");	
	if (NULL == fsrc)
	{
		printf("Can't open %s \n",sDestFileName);
		return false;
	}

	FILE *fdst = fopen(TEMP_FILE_NAME, "w");
	if (NULL == fdst)
	{
		printf("Can't open temporary %s \n", TEMP_FILE_NAME );
		return false;
	}

	int LinesProcessed = 0;

	char sLine[STR_BUFFER_SIZE];
	while (fgets(sLine, STR_BUFFER_SIZE, fsrc) != NULL)
	{
		if (findTokenAndInsertVal(sLine, sTokenToLookup, sTokenValue))
		{
			LinesProcessed++;
		}

		fputs(sLine, fdst);
	}

	fclose(fsrc);
	fclose(fdst);

	_unlink(sDestFileName);
	rename(TEMP_FILE_NAME, sDestFileName);


	return true;
}

int main(int argc,char* argv[])
{

	char* sSourceFileName = getProgramParameter(argc, argv, "-s");
	char* sDestinationFileName = getProgramParameter(argc, argv, "-d");
	char* sTokenToLookup = getProgramParameter(argc, argv, "-t");

	if (sSourceFileName == NULL || sTokenToLookup == NULL)
	{
		printf(" usage: \n    parser.exe -s sourceFile.txt -t TokenToLookup"
			"\n    parser.exe - s sourceFile.txt -d destinationFile.txt -t TokenToLookup\n");
		return 1;
	}

	char sTokenValue[STR_BUFFER_SIZE];
	if (NULL == getSrcTokenValue(sSourceFileName, sTokenToLookup, sTokenValue))
	{
		printf("Can't find token '%s' in source file.\n", sTokenToLookup);
		return 2;
	}	

	if (sDestinationFileName == NULL)
	{
		printf("Token %s = %s\n",sTokenToLookup, sTokenValue);
		return 0;
	}
	
	if (!processResultFile(sDestinationFileName, sTokenToLookup, sTokenValue))
	{
		printf("Can't find token '%s' in destination file.\n", sTokenToLookup);
	}
	else
	{
		printf("Success.\n");
	}

    return 0;
}

