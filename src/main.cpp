//============================================================================
// Name        : lamer.cpp
// Author      : bat.serjo
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================
/*
Write a C/C++ commandline application that encodes a set of WAV files to MP3
Requirements:
(1) application is called with pathname as argument, e.g.
<applicationname> F:\MyWavCollection all WAV-files contained directly in that folder are to be encoded to MP3
(2) use all available CPU cores for the encoding process in an efficient way by utilizing multi-threading
(3) statically link to lame encoder library
(4) application should be compilable and runnable on Windows and Linux
(5) the resulting MP3 files are to be placed within the same directory as the source WAV files, the filename extension should be changed appropriately to .MP3
(6) non-WAV files in the given folder shall be ignored
(7) multithreading shall be implemented by means of using Posix Threads (there exist implementations for Windows)
(8) the Boost library shall not be used
(9) the LAME encoder should be used with reasonable standard settings (e.g. quality based encoding with quality level "good")
*/

#include <ios>
#include <string>
#include <iomanip>
#include <fstream>
#include <iostream>

#include "Wave.h"
#include "Path.h"
#include "TQueue.h"
#include "TManager.h"

#include <lame.h>
#include <pthread.h>

//#define APPEND_EXT

using namespace std;

// Used to report status.
// The worker only writes, the main thread only reads
typedef struct {
	uint32_t totalLen;
	uint32_t curPos;
	uint8_t  fname[64];
} tProgressInfo;


///////////////////////////////////////
// Class to convert a wave file to mp3
///////////////////////////////////////
class WaveToMp3 : public WAVEFile {
private:
	std::string *fileName;
public:
	WaveToMp3(std::string *fname):WAVEFile((char*)fname->c_str()) {
		fileName = fname;
	}

	WaveToMp3( char *fname ) : WAVEFile(fname) {
		fileName = new std::string(fname);
	}

	~WaveToMp3() {
		if (fileName != NULL) delete fileName;
	}

	void toLameMp3(tProgressInfo *prog) {
		static const char* mp3ext = ".mp3";
		uint8_t  	 *mp3buff     = NULL;
		uint32_t      mp3buffSize = 0;
		uint16_t 	  samplesPerIter = 1152*8;
		uint32_t 	  maxLen;

		size_t     	  offset = 0;
		int 		  encodedLen;

		WAVEChannels *wavChans;
		std::ofstream outf;

		/////////////////////////////////////////////

		std::string *base = Path::getBase(*fileName);
		std::string *flnm = Path::getFile(*fileName);

#ifndef APPEND_EXT
		std::string* ext  = Path::getExt (*flnm);

		if ( ext != NULL ) {
			size_t pos = flnm->rfind(*ext);
			flnm->replace(pos, ext->size(), mp3ext);
		} else {
			*flnm += mp3ext;
		}
#else
		*flnm += mp3ext;
#endif

		std::string* mp3name = Path::join(*base, *flnm);

		outf.open(mp3name->c_str(), ios::out|ios::binary|ios::trunc);
		if (!outf.is_open()) {
			throw "Cannot open file for writing";
		}

		lame_t lame = lame_init();

		lame_set_VBR	       (lame, vbr_default);
		lame_set_mode          (lame, STEREO);
		lame_set_quality       (lame, 5);
		lame_set_VBR_quality   (lame, 5);
		lame_set_out_samplerate(lame, this->fmt->fields->dwSamplesPerSec );

		lame_set_num_channels  (lame, this->fmt->fields->wChannels);
		lame_set_in_samplerate (lame, this->fmt->fields->dwSamplesPerSec);
		lame_set_brate		   (lame, (this->fmt->fields->dwAvgBytesPerSec*8) / 1000);
		lame_set_num_samples   (lame, (data->ckSize / fmt->fields->wBlockAlign) / fmt->fields->wChannels);

		lame_init_params(lame);

		// Fill some report information.
		if ( prog != NULL ) {
			prog->curPos   = 0;
			prog->totalLen = data->ckSize;

			maxLen = 64;
			if (flnm->size() < 64) {
				maxLen = flnm->size();
			}

			memcpy(&(prog->fname), flnm->c_str(), maxLen);
			prog->fname[maxLen] = '\0';
		}

		//the buffer a little bigger than 1.25
		mp3buffSize = 2 * samplesPerIter + 7200;
		mp3buff 	= new uint8_t[ mp3buffSize ];

		while( (wavChans = getSamples(offset, samplesPerIter)) && (wavChans != NULL) ) {

			if (sampleSizeInBytes <= 2) {
				encodedLen = lame_encode_buffer( lame,
								(const int16_t*)(*wavChans)[0],
								(const int16_t*)(*wavChans)[1],
								wavChans->getSamplesPerChannel(),
								&mp3buff[0],
								mp3buffSize );
			} else {
				encodedLen = lame_encode_buffer_int( lame,
								(const int*)(*wavChans)[0],
								(const int*)(*wavChans)[1],
								wavChans->getSamplesPerChannel(),
								&mp3buff[0],
								mp3buffSize );
			}

			if ( encodedLen < 0 ) {
				throw "Lame encode returned and error.";
			}

			if ( prog != NULL ) {
				prog->curPos += wavChans->getSamplesPerChannel() * this->fmt->fields->wBlockAlign;
			}

			outf.write((const char*)mp3buff, encodedLen);
			offset += wavChans->getSamplesPerChannel() * this->fmt->fields->wBlockAlign;

			delete wavChans;
		}

		encodedLen = lame_encode_flush(lame, &mp3buff[0], sizeof(mp3buff));
		outf.write((const char*)mp3buff, encodedLen);
		outf.close();

		delete [] mp3buff;
		delete base;
		delete flnm;
		delete mp3name;
	}

};



////////////////////////////////////////////////
// Typedefs for the implementation below.
////////////////////////////////////////////////

typedef TQueue< std::string* > tWavPipe;

typedef enum ConsumerState {
	eWAITING,
	eBUSY,
	eDone
} tConsumerState;


typedef struct {
	tConsumerState 		state;
	tWavPipe 			*pipe;
	tProgressInfo 		progress;
} tConsumerArgs;

typedef struct {
	uint16_t numConsumers;
	char 	 *folder;
	tWavPipe *pipe;
} tProducerArgs;


///////////////////////////////////////////////

WaveToMp3* LoadWavFile( std::string *fname ){
	WaveToMp3 * waf = NULL;

	try {
		waf = new WaveToMp3(fname);
	} catch( char const* e) {
		std::cout <<endl << "Exception: "<< *fname << std::endl<< e<< std::endl;
		std::cout << std::endl;
		return NULL;
	}

	try {
		waf->parseHeader();
//		waf->print();
	} catch (char const* e) {
		std::cout << "Exception:"<< *fname<< ": "<< e<< endl;
		return NULL;
	} catch(...) {
		std::cout << "exception: "<< *fname<< endl;
		return NULL;
	}

	return waf;
}

void* ConsumerThread(void *p) {
	tConsumerArgs *args = (tConsumerArgs*)p;
	int oldstate;
	int oldtype;

	if ( p == NULL ) {
		return NULL;
	}

	args->state = eWAITING;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,  &oldstate);
	pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, &oldtype);

	while(true) {
		args->state = eWAITING;

		// Get some work
		std::string* fname = args->pipe->pop();
		if (fname == NULL) {
			// OK, no more work we die.
			break;
		}

		WaveToMp3 *waf = LoadWavFile( fname );
		if ( waf == NULL ) {
			//something went wrong loading the WAVE file.
			continue;
		}

		// launch the conversion
		args->state = eBUSY;
		try {
			waf->toLameMp3(&(args->progress));
		} catch(char const *e) {
			std::cout <<endl <<"Consumer Exception: "<< e<< endl;
		}

		delete waf;
	}

	args->state = eDone;
	return NULL;
}


void* ProducerThread(void *p) {
	int oldtype;
	int oldstate;

	std::string folderName;
	std::list< std::string* > riffFiles;

	tProducerArgs *args = (tProducerArgs*)p;

	std::list<std::string*>* allFiles = NULL;
	std::list<std::string*>::iterator filesIter;

	///////////////////////////////////////////

	if ( p == NULL ) {
		return NULL;
	}

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,  &oldstate);
	pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, &oldtype);

	allFiles = Path::getFilesInFolder(args->folder);
	if (allFiles == NULL) {
		cout<< "Cannot process folder: "<< args->folder<< endl;
		goto cleanup;
	}

	folderName = args->folder;

	// Filter the files in the folder. Need only RIFFs
	for (filesIter = allFiles->begin(); filesIter != allFiles->end(); filesIter++) {
		std::string *fpath = Path::join( folderName, *(*filesIter) );
		if ( isRiffFile( fpath ) == true ) {
			riffFiles.push_back( fpath );
		}
	}

	// Start the queuing.
	for (filesIter = riffFiles.begin(); filesIter != riffFiles.end(); filesIter++) {
		cout << *(*filesIter)<< endl;
		args->pipe->push( *filesIter );
	}

	cleanup:
	//push NULL to make the consumers quit!
	for (uint16_t i=0; i < args->numConsumers; i++ ) {
		args->pipe->push( NULL );
	}

	return NULL;
}



int main( int argc, char* argv[] )
{
	if (argc < 2) {
		cout << "Usage: "<< argv[0] << " <path to folder>"<<endl;
		return 0;
	}

	// Create a thread manager instance.
	TMan::TManager tman;

	// First find out how many cores this machine has
	uint16_t ncpus = tman.GetNumCPU();
	std::cout<< "Num CPUS: "<< ncpus<< std::endl;

	// guess the number of consumers for this system
	uint16_t numConsumers = ncpus + 1;

	// Create the queue between the producers and consumers;
	tWavPipe* pipe = new tWavPipe(numConsumers) ;

	// Start the producer thread.
	// It will walk the folder given as argument.
	// And will try to open each file as WAV
	// If successful the object will end up in the queque
	tProducerArgs pargs;
	pargs.numConsumers = numConsumers;
	pargs.folder = argv[1];
	pargs.pipe   = pipe;

	tman.ThreadStart(NULL, ProducerThread, (void*)&pargs, 64*1024);

	// wait for some data from producer.
	while( pipe->getLength() == 0 ) {
		TMan::SleepMiliSec(100);
	}

	// We will start the consumer threads
	// Allocate space for the arguments and results.
	TMan::targ *results   = new TMan::targ[ numConsumers ];
	TMan::targ *arguments = new TMan::targ[ numConsumers ];

	tConsumerArgs *cargs;
	for (uint16_t i=0; i < numConsumers; i++) {
		results[i]   = NULL;
		cargs 		 = new tConsumerArgs;
		memset(cargs, 0, sizeof(tConsumerArgs));

		cargs->pipe  = pipe;
		cargs->state = eWAITING;
		arguments[i] = (TMan::targ)cargs;
	}

	// Start a team of consumers.
	tman.TeamStart(numConsumers, results, ConsumerThread, arguments, 256*1024);

	///////////////////////////////////////////////////
	//  At this point everything is up and running.
	///////////////////////////////////////////////////

	// The consumers will report their state in the argument.
	// Our condition for ending everything is:
	// - the length of the queue is 0
	// - all consumers are in state WAITING

	double tmpPrcnt = 0;
	bool bStillWorking = true;

	while ( bStillWorking ) {
		bStillWorking = false;

		cout<<endl;
		for (uint16_t idx = 0; idx < numConsumers; idx++ ) {

			cargs = (tConsumerArgs*)(arguments[idx]);

			if ( cargs->state != eDone ) {
				bStillWorking = true;

				if (cargs->progress.totalLen == 0)
					cargs->progress.totalLen++;

				tmpPrcnt  = cargs->progress.curPos * 100.0;
				tmpPrcnt /= cargs->progress.totalLen;

				cout<< std::setfill(' ')<<"[";
				cout<< std::setw(6)<< std::setprecision(2)<<std::fixed << tmpPrcnt;
				cout<< "%]  # ";
				cout.width(64);
				cout<< std::setfill('.') << cargs->progress.fname<< endl;
			}
		}

		TMan::SleepMiliSec(333);
	}

	tman.ThreadJoin(ProducerThread);
	tman.TeamDelete(ConsumerThread);

	for (uint16_t i=0; i < numConsumers; i++) {
		cargs = (tConsumerArgs*)arguments[i];
		delete cargs;
	}
	delete [] arguments;
	delete [] results;

	delete pipe;
	return 0;
}
