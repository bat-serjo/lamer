/*
 * Path.h
 *
 *  Created on: May 24, 2015
 *      Author: serj
 */

#ifndef PATH_H_
#define PATH_H_

#include <list>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32 
#include <sys/stat.h>
#include <Windows.h>
#else
#include <dirent.h>
#endif

namespace Path {

	inline char GetSep( void );

	void levelUp(std::string& path);

	std::string* join(const char* a, const char* b);
	std::string* join(const std::string& a, const std::string& b);

	std::string* getBase(const char *a);
	std::string* getBase(const std::string& a);

	std::string* getFile(const char *a);
	std::string* getFile(const std::string& a);

	std::string* getExt(const char *a);
	std::string* getExt(const std::string& a);

	std::list<std::string*>* getFilesInFolder(const char* folder);

}

#endif /* PATH_H_ */
