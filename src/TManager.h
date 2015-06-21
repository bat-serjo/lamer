/*
 * TManager.h
 *
 *  Created on: May 23, 2015
 *      Author: serj
 */

#ifndef TMANAGER_H_
#define TMANAGER_H_

#include "stdint.h"
#include "pthread.h"

#include <map>
#include <list>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace TMan {
	typedef void *(*troutine) (void *);
	typedef void * targ;

	typedef pthread_t tTID;

	typedef enum {
		WAIT_ALL,
		WAIT_ANY,
		WAIT_TID
	}twait;


	inline void SleepMiliSec(uint32_t msec) {
#ifdef _WIN32
		Sleep(msec);
#else
		usleep(msec*1000);
#endif
	}

	class Thread {
	private:
		pthread_attr_t attr;

	public:
		tTID 	 tid;
		targ 	 arg;
		targ  	 res;
		troutine tr;
		uint32_t stackSize;

		Thread() {
			memset(&tid, 0, sizeof(tid)); 
            arg = NULL; 
            res = NULL; 
            tr  = NULL;

            stackSize = 8*1024;
            pthread_attr_init(&attr);
		}

		Thread(targ _res, troutine _tr, targ _arg, uint32_t stackSz=64*1024 ) {
			memset(&tid, 0, sizeof(tid)); 
            arg = _arg;  res = _res, tr  = _tr;

            stackSize = stackSz;
            pthread_attr_init(&attr);
		}

		~Thread() {
			pthread_attr_destroy(&attr);
		}

		void Start(void) {
			pthread_attr_setstacksize(&attr, stackSize);
			pthread_create(&tid, NULL, tr, arg);
		}
	};

	class Team {
	public:
		std::list< Thread* > all;

		Team(){}
		~Team(){}
		void TeamAdd( Thread* th ) { all.push_back(th); }
	};

	class TManager {
		private:
			std::map< troutine, Team* >   teams;
			std::map< troutine, Thread* > threads;

		public:
			TManager();
			~TManager();

			uint16_t GetNumCPU();

			void ThreadStart(targ res, troutine tr, targ arg, uint32_t stsz=64*1024);
			void ThreadJoin(troutine tr);
			void ThreadDelete(troutine tr);
			Thread* ThreadGet(troutine tr);

			void TeamStart(uint16_t num, targ *res, troutine tr, targ* args, uint32_t stsz=64*1024);
			void TeamCancel(troutine tr);
			void TeamWait(troutine tr);
			void TeamDelete(troutine tr);
			Team* TeamGet(troutine tr);
	};

}

#endif /* TMANAGER_H_ */
