/*
 * TManager.cpp
 *
 *  Created on: May 23, 2015
 *      Author: serj
 */

#include "TManager.h"

#ifdef _WIN32
#include "windows.h"
#else
#include "unistd.h"
#endif

namespace TMan{

		TManager::TManager(){}

		TManager::~TManager() {
		}

		uint16_t TManager::GetNumCPU() {
			#ifndef _WIN32
			return (uint16_t)sysconf(_SC_NPROCESSORS_ONLN);
			#else
			SYSTEM_INFO info;
			GetSystemInfo(&info);
			return (uint16_t) info.dwNumberOfProcessors;
			#endif
		}

		void TManager::ThreadStart(targ res, troutine tr, targ arg, uint32_t stsz) {
			Thread *tmpThr = new Thread(res, tr, arg, stsz);
			threads[ tr ] = tmpThr;
			tmpThr->Start();
		}

		void TManager::ThreadJoin(troutine tr) {
			Thread *thr = threads[tr];
			pthread_join(thr->tid, &(thr->res));
		}

		void TManager::ThreadDelete( troutine tr ) {
			ThreadJoin(tr);

			Thread *thr = threads[tr];
			delete thr;
			threads.erase(tr);
		}

		Thread* TManager::ThreadGet(troutine tr) {
			return threads[ tr ];
		}



		void TManager::TeamStart(uint16_t num, targ *res, troutine tr, targ* args, uint32_t stsz) {
			Thread *thrd = NULL;
			Team *team = new Team();


			for( uint16_t idx = 0; idx < num; idx++ ) {
				targ tmpResult   = NULL;
				targ tmpArgument = NULL;

				if (res != NULL){
					tmpResult = res[idx];
				}

				if (args != NULL) {
					tmpArgument  = args[idx];
				}

				thrd = new Thread(tmpResult, tr, tmpArgument, stsz);
				thrd->Start();
				team->TeamAdd(thrd);
			}

			teams[tr] = team;
		}

		void TManager::TeamWait(troutine tr) {
			Team *team = teams[tr];
			std::list<Thread*>::iterator thrIt;

			for( thrIt = team->all.begin(); thrIt != team->all.end(); thrIt++ ) {
				pthread_join( (*thrIt)->tid, &((*thrIt)->res) );
			}
		}

		void TManager::TeamCancel(troutine tr) {
			Team *team = teams[tr];
			std::list< Thread* >::iterator thrIt;

			for( thrIt = team->all.begin(); thrIt != team->all.end(); ++thrIt ) {
				 while( pthread_cancel( (*thrIt)->tid ) != 0) {};
			}
		}

		void TManager::TeamDelete( troutine tr ) {
			Team *team = teams[tr];
			std::list< Thread* >::iterator thrIt;

			TeamWait(tr);

			while (team->all.empty() == false) {
				for( thrIt = team->all.begin(); thrIt != team->all.end(); ++thrIt ) {
					delete (*thrIt);
					thrIt = team->all.erase( thrIt );
				}
			}

			delete team;
			teams.erase(tr);
		}

		Team* TManager::TeamGet(troutine tr) {
			return teams[tr];
		}
}
