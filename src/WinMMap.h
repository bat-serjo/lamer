/*
 * WinMMap.h
 *
 *  Created on: May 17, 2015
 *      Author: serj
 */

#ifndef WINMMAP_H_
#define WINMMAP_H_

#ifdef _WIN32
#include <wtypes.h>
#include <winbase.h>
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

// mmap prot flags
#define PROT_NONE   0x00	// no access (not supported on Win32)
#define PROT_READ	0x01
#define PROT_WRITE	0x02

// mmap flags
#define MAP_SHARED	0x01	// share changes across processes
#define MAP_PRIVATE	0x02	// private copy-on-write mapping
#define MAP_FIXED	0x04

#define MAP_FAILED 0

extern void* mmap(void* start, size_t len, int prot, int flags, int fd, off_t offset);
extern int munmap(void* start, size_t len);
#endif

#endif /* WINMMAP_H_ */
