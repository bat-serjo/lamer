/*
 * Wave.cpp
 *
 *  Created on: May 17, 2015
 *      Author: serj
 */

#include "Wave.h"
#include <fstream>

static void log(const char *fmt, ...) {
#ifdef DEBUG
	printf(fmt);
#endif
}

bool isRiffFile(std::string *fname) {
	bool res 	 = false;
	uint32_t hdr = 0;
	std::ifstream inf;

	inf.open( fname->c_str(), std::ios::binary | std::ios::in );

	if (inf.is_open()) {
		inf.read( (char*)&hdr, 4 );
		if ( RIFFconst::ID_RIFF == be32_to_cpu(hdr) ) {
			res = true;
		}
		inf.close();
	}

	return res;
}

// Used to represent the WAVE entry in the RIFF file.
// It's fake has no length
WAVEChunk::WAVEChunk(uint8_t *addr):FChunk(addr) {
	this->ckData = &this->addr[4];
	this->ckSize = 0;
}

// WAVE FMT
WAVEFmtChunk::WAVEFmtChunk( uint8_t* addr ): FChunk(addr) {
	this->fields = (struct Common *)&this->ckData[0];

	//must be even !
	if ( (this->ckSize & 0x01) != 0 ) {
		this->ckSize++;
	}
}

void WAVEFmtChunk::print () {
	FChunk::print();
	printf( "\twFormatTag:       %hd\n"
			"\twChannels:        %hd\n"
			"\tdwSamplesPerSec:  %d\n"
			"\tdwAvgBytesPerSec: %d\n"
			"\twBlockAlign:      %hd\n"
			"\twBitsPerSample:   %hd\n",
			this->fields->wFormatTag,
			this->fields->wChannels,
			this->fields->dwSamplesPerSec,
			this->fields->dwAvgBytesPerSec,
			this->fields->wBlockAlign,
			this->fields->wBitsPerSample );
}


// Channels
WAVEChannels::WAVEChannels(uint16_t numChannels, uint8_t sampleSzInBytes, uint32_t numSamples) {
		holder 	  = NULL;
		holder 	  = new uint8_t*[ numChannels ];

		this->numChannels = numChannels;
		this->chanSize    = numSamples * sampleSzInBytes;
		this->numOfSamplesInEach = numSamples;

		for( uint16_t idx=0; idx < numChannels; idx++ ) {
			holder[idx] = new uint8_t[ this->chanSize ];
			memset(holder[idx], 0, this->chanSize);
		}
	}

WAVEChannels::~WAVEChannels() {
	if (holder != NULL) {
		for (uint16_t chan=0; chan < numChannels; chan++ ) {
				delete [] holder[chan];
		}
		delete [] holder;
	}
}

uint16_t WAVEChannels::getNumChannels() {
	return numChannels;
}

uint32_t WAVEChannels::getChannelBufSize() {
	return chanSize;
}

uint32_t WAVEChannels::getSamplesPerChannel() {
	return numOfSamplesInEach;
}

uint8_t* WAVEChannels::operator[](uint16_t idx) {
	if (idx < numChannels ) {
		return holder[idx];
	} else {
		return NULL;
	}
}


////////////////////////////////////////////
// class to handle a Wave file
////////////////////////////////////////////

WAVEFile::WAVEFile( char *fname ) {
	if ( fname == NULL ) { throw "WAVEFile filename is NULL"; }

	this->fname = fname;
	this->mapper = new MMapFile();
	this->mapper->openFile( this->fname, (char*)"rb" );

	this->fmt  = NULL;
	this->data = NULL;
	this->sampleSizeInBytes = 0;
}

WAVEFile::~WAVEFile() {
	if ( this->mapper != NULL)
		delete this->mapper;
	if ( this->data != NULL )
		delete this->data;
	if ( this->fmt  != NULL )
		delete this->fmt;
}

void WAVEFile::parseHeader()
{
	size_t lastAddr;

	MMapFile *map = this->mapper;
	lastAddr = ((size_t)map->getAddr() + map->getSize());

	FChunk riffChunk = FChunk( map->getAddr() );
	if ( riffChunk.ckID   != RIFFconst::ID_RIFF ||
		 riffChunk.ckSize  > map->getFileSize() ) {
		log("%x != %x size: %d\n",cpu_to_be32( riffChunk.ckID ),
						 RIFFconst::ID_RIFF,
						 riffChunk.ckSize );
		throw "Bad or not a RIFF file format.";
	}

	uint8_t *subAddr = riffChunk.getSub();
	if ( subAddr > (uint8_t*)lastAddr ) {
		throw "Bad or not a RIFF file format.";
	}

	FChunk waveChunk = WAVEChunk( subAddr );
	if ( waveChunk.ckID  != WAVEconst::ID_WAVE ) {
		log(" WAVE: %x %x sz: %d %d \n", waveChunk.ckID, WAVEconst::ID_WAVE, waveChunk.ckSize, riffChunk.ckSize);
		throw "Bad or not a WAVE file.";
	}

	// walk the wave chunk for all sub chunks
	// and search for fmt and data.
	for( subAddr = waveChunk.getSub();
		 subAddr < (uint8_t*)lastAddr; )
	{
		FChunk curChunk = FChunk( subAddr );

		if ( curChunk.ckID == WAVEconst::ID_FMT ) {
			this->fmt  = new WAVEFmtChunk( subAddr );
		}

		if ( curChunk.ckID == WAVEconst::ID_DATA ) {
			this->data = new FChunk( subAddr );
		}

		subAddr = curChunk.getNext();
	}

	this->checkSanity();
}

// Is this wave file sane. Can we handle it?
void WAVEFile::checkSanity() {
	// General checks
	if ( this->fmt == NULL || this->data == NULL ) {
		throw "fmt OR data chunk not found!";
	}

	if (this->fmt->fields->wFormatTag != WAVEconst::M_WAVE_FORMAT_PCM &&
		this->fmt->fields->wFormatTag != WAVEconst::M_WAVE_FORMAT_EXTENSIBLE) {
		throw "Sorry this wave file has strange data format.";
	}

	// FMT checks
	uint16_t u16BPS = this->fmt->fields->wBitsPerSample;
	if ( u16BPS > 64 ){
		throw "Bits per sample not quite sane.";
	}
	uint8_t mod = u16BPS % 8;
	this->sampleSizeInBytes = (u16BPS + mod) / 8;

	if ( (this->sampleSizeInBytes * this->fmt->fields->wChannels) != this->fmt->fields->wBlockAlign ) {
		throw "Block align does not match the channels and bits per sample.";
	}

	// DATA checks
	if ( this->data->ckSize > this->mapper->getFileSize() ) {
		throw "Data size is wrong.";
	}

	if ( (this->data->ckSize % this->fmt->fields->wBlockAlign) != 0 ) {
		throw "Data is truncated!";
	}
}

//
// Will validate the access attempt.
// Will check if numSamples can be satisfied.
// If not the actual number of usable samples will be returned.
//
// Returns tRawDataResult
//
WAVEFile::tRawDataResult WAVEFile::getRawChannelsData( size_t offset, uint16_t numSamples ) {
	size_t endOfRequest = 0;
	size_t bytesToRead = numSamples * this->fmt->fields->wBlockAlign;
	tRawDataResult ret;

	if ( (offset % this->fmt->fields->wBlockAlign) != 0 ) {
		throw "Unaligned to block offset used to access the data.";
	}

	endOfRequest = offset + bytesToRead;

	if ( endOfRequest > this->data->ckSize ) {
		// something is apparently wrong the request reaches outside data size.
		endOfRequest -= (endOfRequest - this->data->ckSize);
		// figure out how many samples are in the trimmed chunk.
		numSamples = (endOfRequest - offset) / (this->fmt->fields->wBlockAlign);
	}

	ret = std::make_pair(&this->data->ckData[offset], numSamples);
	return ret;
}

// Returns object of WAVEChannels or NULL
// THE CODE CONSUMING THE THIS MUST FREE THE MEMORY !!!
WAVEChannels* WAVEFile::getSamples( size_t offset, uint16_t numSamples ) {
	WAVEChannels* 	channels = NULL;
	tRawDataResult 	paired;

	uint8_t*		data 	= NULL;
	uint8_t*		curData = NULL;
	uint8_t 		retChanSampleSizeInBytes;

	uint16_t* 	u16Ptr = NULL;
	uint32_t*	u32Ptr = NULL;
	uint32_t 	u32Var;

	paired = this->getRawChannelsData(offset, numSamples);
	data 		= paired.first;
	numSamples 	= paired.second;

	// getRawChannelsData will return 0 samples available when all data is consumed
	if (numSamples == 0) {
		return channels;
	}


	// since the wave file can be 8bit or 24bit or whatever
	// some scaling must be done between reading the RAW data
	// and pushing it into the channels object.

	retChanSampleSizeInBytes = sampleSizeInBytes;

	if ( sampleSizeInBytes == 1 ) {
		retChanSampleSizeInBytes = 2;
	}

	if ( sampleSizeInBytes == 3 ) {
		retChanSampleSizeInBytes = 4;
	}

	//We can't fail after this point. Except for memory allocation.
	channels = new WAVEChannels( fmt->fields->wChannels,
								 retChanSampleSizeInBytes,
								 numSamples);

	for( uint16_t curSample = 0; curSample < numSamples; curSample++ )
	{
		curData = &data[ curSample * fmt->fields->wBlockAlign ];

		for( uint16_t ch=0; ch < fmt->fields->wChannels; ch++ ) {

			uint8_t *curChanBuff = (*channels)[ch];
			uint8_t *curBuffPos  = &curChanBuff[ curSample * retChanSampleSizeInBytes ];

			u16Ptr = (uint16_t*)curBuffPos;
			u32Ptr = (uint32_t*)curBuffPos;

			switch( sampleSizeInBytes ) {
			case 1:
				// oh the madness !
				*u16Ptr = ((((int8_t)curData[ch * sampleSizeInBytes]) - 128) << 8);
				break;
			case 2:
				*u16Ptr = le16_to_cpu( *(uint16_t*)&curData[ch * sampleSizeInBytes] );
				break;
			case 3:
				u32Var = le32_to_cpu(*(uint32_t*)&curData[ch * sampleSizeInBytes]); // ch * 3
				u32Var = u32Var << 8;
				*u32Ptr = u32Var;
				break;
			case 4:
				u32Var  = *(uint32_t*)&curData[ch * sampleSizeInBytes];
				*u32Ptr = le32_to_cpu( u32Var );
				break;
			}
		}

	}

	return channels;
}

void WAVEFile::print() {
	this->mapper->print();
	this->fmt->print();
	this->data->print();
}
