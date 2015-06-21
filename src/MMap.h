/*
 * MMap.h
 *
 *  Created on: May 17, 2015
 *      Author: serj
 */

#ifndef MMAP_H_
#define MMAP_H_

#include "stdio.h"
#include "stdint.h"

#ifdef _WIN32
#include "WinMMap.h"
#endif


class MMapFile {
private:
	FILE* 	ptrFileDesc;
	char*	fname;
	int 	fd;
	uint8_t*addr;
	size_t  length;
	size_t  fileLength;

public:
	MMapFile();
	~MMapFile();

	void 	 openFile( char *fname, char* mode, int fixedsize = 0 );
	uint8_t* getAddr();
	size_t   getSize();
	size_t   getFileSize();
	void 	 print();
};


#endif /* MMAP_H_ */
