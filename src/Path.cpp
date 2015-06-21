/*
 * Path.cpp
 *
 *  Created on: May 24, 2015
 *      Author: serj
 */

#include "Path.h"


namespace Path {

inline char GetSep( void ) {
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}

void levelUp(std::string& path) {
	size_t idx = 0;
	idx = path.rfind(GetSep());

	if ( idx != std::string::npos ) {
		path.erase(idx);
	}
}

std::string* join(const char* a, const char* b) {
	std::string sa(a);
	std::string sb(b);
	return join(sa, sb);
}
std::string* join(const std::string& a, const std::string& b){
	std::string* ret = new std::string;
	*ret += a;
	*ret += GetSep();
	*ret += b;
	return ret;
}

std::string* getBase(const char *a) {
	std::string sa(a);
	return getBase(sa);
}

std::string* getBase(const std::string& a) {
	std::string tmp(a);
	levelUp( tmp );
	return new std::string( tmp );
}

std::string* getFile(const char *a) {
	std::string sa(a);
	return getFile(sa);
}

std::string* getFile(const std::string& a) {
	size_t idx;

	idx = a.rfind( GetSep() );
	if ( idx != std::string::npos ) {
		return new std::string( a.substr(++idx) );
	}

	return new std::string(a.substr(0));
}

std::string* getExt(const char *a) {
	std::string sa(a);
	return getExt(sa);
}

std::string* getExt(const std::string& a) {
	size_t extIdx;

	extIdx = a.rfind('.');
	if (extIdx != std::string::npos) {
		return new std::string(a.substr(extIdx));
	}

	return NULL;
}

#ifndef _WIN32
static std::list<std::string*>* _unixGetFilesInFolder( const char* folder) 
{
	std::list<std::string*>* res = new std::list<std::string*>();
    struct dirent *ep;

    DIR *dp = opendir( folder );
    if (dp == NULL)	{
        return NULL;
    }

    do {
        ep = readdir( dp );
        if ( ep == NULL )
            break;

		std::string *tmp = new std::string(ep->d_name);
		res->push_back(tmp);
    } while(true);

    (void)closedir(dp);

    return res;
}
#else
static std::list<std::string*>* _winGetFilesInFolder(const char* folder) {
	std::list<std::string*>* res = new std::list<std::string*>();
	std::string* fldr;

	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
	DWORD dwError=0;
   
	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.

	if (strlen(folder) > (MAX_PATH - 3)) {
		return NULL;
	}
   
	fldr = new std::string(folder);
	*fldr += "\\*";

	// Find the first file in the directory.
	hFind = FindFirstFile(fldr->c_str(), &ffd);
	if (INVALID_HANDLE_VALUE == hFind)  {
		delete fldr;
		return NULL;
	} 
   
	do {
		if ( (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {
			std::string *tmp = new std::string(ffd.cFileName);
			res->push_back(tmp);
		}
	} while (FindNextFile(hFind, &ffd) != 0);
 
	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES) {
		throw "Cannot enumerate files in folder";
	}

	FindClose(hFind);
	return res;
}
#endif

std::list<std::string*>* getFilesInFolder(const char* folder) {
#ifdef _WIN32
	return _winGetFilesInFolder(folder);
#else
	return _unixGetFilesInFolder(folder);
#endif
}

}
