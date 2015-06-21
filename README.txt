This README is supposed to give general overview on the program implementation and the decisions that influenced it.   

	- What is supported: PCM 8,16,24,32 also partial bit lengths (18,20 bit) should work but not tested.
	- What is not supported: a-law, u-law, floats, 64bit PCM and other known and unknown data formats.
	  However these other data formats can be easily added.
	- At most the first 2 channels will be encoded in the mp3. No mixing. 
	  
	- PROS: 
		I tried to make the program CPU byte order agnostic, however I have no access to big endian machine to prove it.
		Processed files are mmaped in memory this should give a very very slight advantage due to reduced number of system calls.
	
	- CONS:
		Processed files are mmaped in memory. That means the program can effectively run out of memory if you process big enough files
		This potential problem could be solved in two different ways 
		- reduce the size of the mmaped chunk of the file
		- resolve to the conventional readin from the file 
		There are still a few memory leaks left in the path/file name handling area. However since these are
		needed through almost the whole life span to the program I decided not to fix them. Lame also leaks some memory.
		The extended part of the FMT chunk is not used. No other WAVE chunks besides FMT and DATA are taken into account.
	
	- INFO:
		There are projects for both Eclipse and Visual Studio 10 in the folder. Due to this I did not bother to create
		separate makefile for building. 
		
		Under Linux you need to install the libmp3lame package for your distribution. The expectations of the Eclipse
		project are that it will find the headers in the normal place /usr/include/lame and the lame archive in 
		/usr/lib/x86_64-linux-gnu/libmp3lame.a If your paths are different please modify the project accordingly.
		
		Under Winwows the project takes all that is needed from the sub folders lame-3.99.5 and pthreads4w-code.
		So there should be no need to change anything.
		
		If you decide to compile using cygwin simply do  
		g++ -Wall -o "lamer"  ./src/*.cpp -I <path to lame headers> <path to>/libmp3lame.a -lpthread
