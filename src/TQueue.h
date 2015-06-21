/*
 * TQueue.h
 *
 *  Created on: May 17, 2015
 *      Author: serj
 */

#ifndef TQUEUE_H_
#define TQUEUE_H_

#include <vector>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

// Blocking queue with size limit.

template <typename T>
class TQueue {
private:
    std::vector<T> 	_array;
    uint32_t		maxSize;
    pthread_mutex_t mlock;
    pthread_cond_t 	rcond;
    pthread_cond_t  wcond;
    size_t		    _len;


public:
   TQueue (uint32_t size) : maxSize(size) {
       pthread_mutex_init(&mlock, NULL);
       pthread_cond_init(&rcond, NULL);
       pthread_cond_init(&wcond, NULL);
       _array.reserve(maxSize);
       _len = 0;
    }

   ~TQueue ( ) {
       pthread_mutex_destroy(&mlock);
       pthread_cond_destroy(&rcond);
       pthread_cond_destroy(&wcond);
    }

   void push(const T& value ) {
          pthread_mutex_lock(&mlock);
          while (_array.size( ) == maxSize) {
              pthread_cond_wait(&wcond, &mlock);
          }

          _array.push_back(value);
          _len = _array.size();

         pthread_mutex_unlock(&mlock);
		 pthread_cond_broadcast(&rcond);
   }

   T pop( ) {
          pthread_mutex_lock(&mlock);
          while( _array.empty() ) {
              pthread_cond_wait(&rcond, &mlock) ;
          }

          T _temp = _array.front( );
          _array.erase( _array.begin( ) );
          _len = _array.size();

          pthread_mutex_unlock(&mlock);
		  pthread_cond_broadcast(&wcond);

          return _temp;
   }

   size_t getLength() {
	   return _len;
   }
};

#endif /* TQUEUE_H_ */
