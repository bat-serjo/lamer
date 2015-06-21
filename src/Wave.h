/*
 * Wave.h
 *
 *  Created on: May 17, 2015
 *      Author: serj
 */

#ifndef WAVE_H_
#define WAVE_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <string>
#include <utility>

#include "MMap.h"
#include "ByteOrder.h"

namespace RIFFconst {
	uint32_t const ID_RIFF = 0x52494646; /* "RIFF" */
}

namespace WAVEconst {
	uint32_t const ID_WAVE = 0x57415645; /* "WAVE" */
	uint32_t const ID_FMT  = 0x666d7420; /* "fmt " */
	uint32_t const ID_DATA = 0x64617461; /* "data" */

	uint16_t const M_WAVE_FORMAT_PCM 		= 0x0001;
	uint16_t const M_WAVE_FORMAT_IEEE_FLOAT = 0x0003;
	uint16_t const M_WAVE_FORMAT_ALAW 	    = 0x0006;
	uint16_t const M_WAVE_FORMAT_MULAW 	    = 0x0007;
	uint16_t const M_WAVE_FORMAT_EXTENSIBLE = 0xFFFE;
}

bool isRiffFile(std::string *fname);

class FChunk {
public:
	uint8_t* addr;
	uint32_t ckID;
	uint32_t ckSize;
	uint8_t* ckData;

	// FChunk
	FChunk(uint8_t* addr) {
		 this->addr   = addr;
		 this->ckID   = be32_to_cpu( *(uint32_t*)&addr[0] );
		 this->ckSize = *(uint32_t*)&addr[4];
		 this->ckData =  (uint8_t*) &addr[8];
	}

	virtual uint8_t* getNext() {
		return &this->ckData[ this->ckSize ];
	}

	virtual uint8_t* getSub() {
		return &this->ckData[ 0 ];
	}

	virtual void print() {
		uint32_t rawId = cpu_to_be32(this->ckID);
		char *id = (char*)&rawId;
		printf("FChunk @ %p ID: %c%c%c%c Size: %d \n",
				this->addr,
				id[0], id[1], id[2], id[3],
				this->ckSize);
	}

	virtual ~FChunk (){}
};

class WAVEChunk : public FChunk {
public:
	WAVEChunk(uint8_t *addr);
};

class WAVEFmtChunk : public FChunk {
public:
	struct Common {
		uint16_t wFormatTag;
		uint16_t wChannels;
		uint32_t dwSamplesPerSec;
		uint32_t dwAvgBytesPerSec;
		uint16_t wBlockAlign;
		uint16_t wBitsPerSample;
	} *fields;

	WAVEFmtChunk( uint8_t* addr );
	void print ();
};



////////////////////////////////////////////
// Channel interface
////////////////////////////////////////////
class WAVEChannels {
private:
	uint8_t **holder;
	uint16_t numChannels;
	uint32_t chanSize;
	uint32_t numOfSamplesInEach;

public:

	WAVEChannels(uint16_t numChannels, uint8_t sampleSzInBytes, uint32_t numSamples);
	~WAVEChannels();

	uint16_t getNumChannels();
	uint32_t getChannelBufSize();
	uint32_t getSamplesPerChannel();
	uint8_t* operator[](uint16_t idx);
};


////////////////////////////////////////////
// class to handle a Wave file
////////////////////////////////////////////

class WAVEFile {
protected:
	MMapFile *mapper;
	char 	 *fname;

	WAVEFmtChunk *fmt;
	FChunk 		 *data;

	//holds the size of the container of 1 sample in bytes.
	uint8_t	sampleSizeInBytes;

	typedef std::pair<uint8_t*, uint16_t> tRawDataResult;
public:
	WAVEFile( char *fname );
	virtual ~WAVEFile();

	// Actually parse the file
	void parseHeader();

	// Is this wave file sane. Can we handle it?
	void checkSanity();

	//
	// Will validate the access attempt.
	// Will check if numSamples can be satisfied.
	// If not the actual number of usable samples will be returned.
	//
	// Returns tRawDataResult
	//
	tRawDataResult getRawChannelsData( size_t offset, uint16_t numSamples = 1152 );

	// Returns object of WAVEChannels or NULL
	// THE CODE CONSUMING THE THIS MUST FREE THE MEMORY !!!
	WAVEChannels* getSamples( size_t offset, uint16_t numSamples = 1152 );

	void print();
};


#endif /* WAVE_H_ */
