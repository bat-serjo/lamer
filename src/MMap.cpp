/*
 * MMap.cpp
 *
 *  Created on: May 17, 2015
 *      Author: serj
 */

#include "MMap.h"
#include <stdlib.h>

#ifndef _WIN32
	//for mmap
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <sys/types.h>
#else
	#include "WinMmap.h"
#endif



MMapFile::MMapFile() {
	this->ptrFileDesc 	= NULL;
	this->fname 		= NULL;
	this->addr 			= NULL;
	this->fd   			= -1;
	this->length 		= 0;
	this->fileLength    = 0;
}

MMapFile::~MMapFile() {
	if ((this->ptrFileDesc != NULL) &&
		(this->fname != NULL)&&
		(this->fd 	 != -1)  &&
		(this->length!= 0)   &&
		(this->addr  != NULL) )
	{
		munmap((void*)this->addr, this->length);
		fclose(this->ptrFileDesc);

		this->fname  = NULL;
		this->fd 	 = -1;
		this->length = 0;
		this->addr   = NULL;
	}
}

void MMapFile::openFile( char *fname, char* mode, int fixedsize )
{
	int fd;
	int ret;
	FILE *fdesc = NULL;
	void *addr  = NULL;
	struct stat st = {0};

	fdesc = fopen(fname, mode);
	if (fdesc == NULL) {throw "Cannot open file";}
	fd = fileno(fdesc);

	ret = fstat(fd, &st);
	if (ret < 0) { close(fd); throw "Cannot stat file"; }

	if (fixedsize == 0) {
		fixedsize = st.st_size;
	}

	addr = mmap(NULL, fixedsize, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if ( addr == MAP_FAILED ) {
		close(fd);
		throw "Cannot mmap file in memory";
	}

	this->ptrFileDesc 	= fdesc;
	this->fname 		= fname;
	this->fd 			= fd;
	this->addr 			= (uint8_t*) addr;
	this->length 		= fixedsize;
	this->fileLength    = st.st_size;
}



uint8_t* MMapFile::getAddr() {
	return this->addr;
}

size_t MMapFile::getSize() {
	return this->length;
}

size_t MMapFile::getFileSize() {
	return this->fileLength;
}

void MMapFile::print() {
	printf("File %s \n@ %p MapLen: %8ld FileLen: %8ld\n",
			this->fname, this->addr, this->length, this->fileLength);
}
