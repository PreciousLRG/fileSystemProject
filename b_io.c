/**************************************************************
* Class:  CSC-415-03 Fall 2023
* Names: Angelo, Bryan, Natalie, Precious, Westley
* Student IDs: 921008618, 916956188, 920698945, 921582542, 922841683 
* GitHub Name: westleyc30
* Group Name: Blob
* Project: Basic File System
*
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "parsePath.h"
#include "directories.h"
#include "mfs.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
	{
	/** TODO add all the information you need in the file control block **/
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
	int rCurrentBlock; //Needed to shift from the exact block we start to the current for the file
	int blockOffset; // This is the actual field we use for the current block to be passed into LBAread
	int byteInBlock; // Used as the offset for reading into the users buffer
	size_t totalBytesRead; //Helps us figure out when we are end of file or count above file size
	int unReadBytes; //The amount of bytes that have not been read yet in our buffer


	//----------------------Write members-----------------------------------------------------
	char * writeBuf;   // represents the state of the current block in disk we are writing to. 
					   // we use this to keep track of everything we need to write AGAIN when doing LBA write 
					   // we use the buffer in the scenario where we need to write less than 512 bytes to our block
					   // since lbaWrite can only write 1 block(512bytes) at minimum then we need to save the state of 
					   // the current block we are writing in append the new stuff to our buffer and overwrite the current 
					   // block with the buffer(which has everything we had saved from the block + the new stuff)

	int curBufPos;        // holds the current position in the buffer -> we check if this had reached 
					        // 512 or not to indicate if the current block we are on has been totally filled 
					        // if this does = 512 then we need to call getNextBlock 
	int bytesLeftInBuffer;	//holds how many bytes are left in OUR buffer that have not been written, so 512 - currBuffPos
	int wCurrentBlock;      // the current block we are writing to 
	//! dont need this i think -> int writeBlockOffset; //Help us keep rack of which file control block we are in
	int userBufPos; // Used as the offset for reading into the USERS buffer
					 // example case: if(unwrittenBytes > bytesLeftInBuffer)
					 //               our currBuffer position is at 400, the user asks to write 200 bytes
					 //               we memcopy (512-currBuffPos) bytes from the USER buffer into OUR buffer 
					 //               totalBytesWritten = (512-currBuffPos) bytes 
					 //               we move the userBuffPos to += totalBytesWritten
					 //               our Buffer is now full so LBAwrite(ourbuffer) 
					 //               currBuffPos = 512
					 //               userBuffPos 
	size_t totalBytesWritten; //Helps us figure out when we are end of file or count above file size
	//! dont need this either bc its the same as bytesLeftInBuffer -> int unWrittenBytes; //The amount of bytes that have not been read yet in our buffer

	//---------------------Shared members------------------------------------------------------

	char fileName[64]; //Helps to find the file based on the given name
	int fileSize; //Helps to track if we're near the end of file
	directoryEntry * DE; //Gives us access to the block start, file size, and the name of the file
	directoryEntry * parent;
	ppInfo * ppi;
	int ROnlyIndic; //0 == false or 1 == true for ROnLY
	int WOnlyIndic; //0 == false or 1 == true for WOnly
	int RWOnlyIndic;//0 == false or 1 == true for RWOnly

	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized



// create flag helper function:
directoryEntry * handleCreateFlag(ppInfo * ppi){
	// create flag was set
	// we want to get an empty DE in the directory the user wants 
	// to make the file in 

	// we need to get the first free block in freespace in order to 
	// allocate some blocks for the file the user wants to create 

	// file is new so the size is going to be 0

	// and then we can fill in the location field for the new files
	// DE
	// first empty block of file -> int location;

	// 1. find free DE in parent
	directoryEntry * parent = ppi->parent;
	int freeIndex = findFreeDirEntry(parent); // we get back the index of free DE
	directoryEntry * newDE = &(parent[freeIndex]); //new DE for file
	ppi->index = freeIndex;
	time_t t = time(NULL);
	// fill in fields for DE:
	newDE->location = allocateBlocks(1);
	strncpy(newDE->name, ppi->lastElement, strlen(ppi->lastElement));
	newDE->size = 512;
	newDE->created = t;
	newDE->modified = t;
	newDE->accessed = t;
	newDE->isDir = 0;

	//We write the file to disk for it to be stored in our file system
	writeDir(ppi->parent);
	return newDE;
}

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags)
	{
	if (startup == 0) b_init();  //Initialize our system

	ppInfo * ppi = malloc(sizeof(ppInfo));
	int ppReturn = parsePath(filename, ppi); //Helps us traverse to the filename
	char * readBuffer = malloc(B_CHUNK_SIZE); //Use separate clean buffer for read
	char * writeBuffer = malloc(B_CHUNK_SIZE); //Use separate clean buffer for write
	
	// get our own file descriptor
	// check for error - all used FCB's
	directoryEntry * DE = ppi->parent;
	// checking flags:
	if(flags & O_CREAT){
		// if parse path returns -1 create it by calling helper function
		if(ppi->index == -1){
			printf("We found that no file exist\n");
			DE = handleCreateFlag(ppi);
		}
		// otherwise open it by returning the fd for that file 
	}

	if(flags & O_TRUNC){
		// truncate flag was set
		// call fs_delete because there is no option to how much you want 
		// to truncate 
		// whether the file exists or doesnt exist the filesize will be zero 
		// if we called 
		// create and truncate -> start a new file or overwritng a new file 
		// in both cases the filesize would be 0
		// how to trunacte file: 
		// if the file exists we call fs_delete 
		// (we will know this from calling parsePath)
		// if it does not exists then we create a new file 
		// in both cases the size of the file will be zero

		if(ppi->index == -1){
			// the file does not exist 
			printf("file does not exist to truncate\n");
			return -1;
		}else{
			//Used to free any blocks the file we're truncating took up
			int ret = fs_delete(filename);

			//Catch case if fs_delete did not work properly
			if(ret == -1){
				printf("fs delete did not work\n");
			}
		}

	}

	
	if ((flags & (O_TRUNC | O_CREAT)) == (O_TRUNC | O_CREAT)) {
    	// Both O_TRUNC and O_CREAT are set
		// 1. check if file exists with ppi
		if(ppi->index == -1){
			// the file does not exist 
			DE = handleCreateFlag(ppi);
		}else{
			int ret = fs_delete(filename);
			if(ret == -1){
				printf("fs_delete did not work\n");
			}
			DE = handleCreateFlag(ppi);
		}
	}

										// check for error - all used FCB's
	b_io_fd fd = b_getFCB();
	if(fd == -1){
		printf("No free file control block for the current file descriptor\n");
		return -1;
	}

	// Initialize fields for fcb:
	if((flags & O_RDONLY) == 0)
		{
			DE = &(ppi->parent[ppi->index]);
			fcbArray[fd].ROnlyIndic = 1;
		}


	//Shared members for read and write
	fcbArray[fd].DE = DE;
	fcbArray[fd].ppi = ppi;
	// fcbArray[fd].parent = ppi->parent;
	// at the end we need to rewrite DE so that 
	//  fcbArray[fd].DE->fileSize = fcbArray[fd].fileSize
	fcbArray[fd].fileSize = fcbArray[fd].DE->fileSize;
	fcbArray[fd].ROnlyIndic = 0; 
	fcbArray[fd].WOnlyIndic = 0; 
	fcbArray[fd].RWOnlyIndic = 0;
	
	fcbArray[fd].buf = readBuffer; //Helps to store a buffer for each given fd
	fcbArray[fd].index = 0;		
	fcbArray[fd].buflen = B_CHUNK_SIZE;		
	fcbArray[fd].rCurrentBlock = DE->location; 
	strncpy(fcbArray[fd].fileName, DE->name, strlen(DE->name));
	fcbArray[fd].unReadBytes = 0; 
	fcbArray[fd].blockOffset = DE->location; // the current block we are on for b_read
	fcbArray[fd].totalBytesRead = 0;
	fcbArray[fd].byteInBlock = 0;

	//Separated instantiated values for open
	fcbArray[fd].writeBuf = writeBuffer; // represents the state of the current 
											// block in disk we are writing to 
										 // we use this to 
	fcbArray[fd].curBufPos = 0;		//holds the current position in the buffer
	fcbArray[fd].bytesLeftInBuffer = B_CHUNK_SIZE; //holds how many valid bytes 
												 // are in the buffer, 
												//clean space left in our buffer
	fcbArray[fd].wCurrentBlock = DE->location; //Needed to shift from the exact 
									//block we start to the current for the file
	fcbArray[fd].userBufPos = 0; // Used as the offset for reading 
								// into the users buffer
	fcbArray[fd].totalBytesWritten = 0; //Helps us figure out when we are 
										//end of file or count above file size

	if(flags & O_APPEND){
		// append flag was set
		// preset the pointer to the end of the filesize aka update the fcb 
		//  pointer to point to the end of the file 
		fcbArray[fd].curBufPos = fcbArray[fd].DE->fileSize;
		fcbArray[fd].byteInBlock = fcbArray[fd].DE->fileSize;
	}


	//---------------------------Read and Write flags--------------------------------

	/**
	 * If any of these flags were to set to 1,
	 * it will help to indicate which of the following
	 * functions to be used only, and those flags
	 * would prevent them from being used
	*/
	
	if(flags & O_WRONLY)
		{
			fcbArray[fd].WOnlyIndic = 1;
		}

	if(flags & O_RDWR)
		{
			fcbArray[fd].RWOnlyIndic = 1;
		}

	return (fd);						// all set
	}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence){

    /**
	 * Changes the position of the opened file
	 * Whence values:
	 * Beginning: Starting block for the file
	 * Current: Current block for the file
	 * End: End of file
	*/

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	

	if(whence & SEEK_CUR){
		printf("SEEK_CUR\n");
		int total = fcbArray[fd].byteInBlock + offset;

		// the case where we will go over our buffer so we need to load more 
		// blocks (our buffer is 512 bytes)
		if(total > B_CHUNK_SIZE){
			// we are going to need to read in a new block
			// set our buffer pointer to 0 bc when we later read and write
			// new blocks will be loded into our buffer
			// for reading, this will trigger a new block to be read  
			fcbArray[fd].unReadBytes = 0; 
			fcbArray[fd].curBufPos = B_CHUNK_SIZE; 

			//position byteInBlock to the correct position 
			int overflow = offset - fcbArray[fd].unReadBytes;

			//checking if we need to move more than 1 block into the 
			//file chain to move the file pointer to where the user wants
			if(overflow > B_CHUNK_SIZE){
				// min amount of times we need to call getNextBlock
				// to put position pointer to offset
				int minBlock = (overflow + (B_CHUNK_SIZE-1))/B_CHUNK_SIZE;
				int remainder = overflow % B_CHUNK_SIZE;
				for(int i = 0; i < minBlock; i++){
					int nextBlock = getNextBlock(fcbArray[fd].blockOffset);
					if(nextBlock == -2){
						nextBlock = appendBlocks(fcbArray[fd].DE,2);
					}
					fcbArray[fd].blockOffset = nextBlock;
					fcbArray[fd].wCurrentBlock = nextBlock;
				} 

				fcbArray[fd].byteInBlock = remainder;
				fcbArray[fd].totalBytesRead += offset;
				fcbArray[fd].totalBytesWritten += offset;
				return fcbArray[fd].totalBytesRead; 
			} // once this if statement executes the block we need to load
			// will be the correct one for LBAread/write
			// update the fields to prepare for call to read or write 
			// with the position specified by seek
			fcbArray[fd].byteInBlock += (overflow % B_CHUNK_SIZE);
			fcbArray[fd].totalBytesRead += offset;
			fcbArray[fd].totalBytesWritten += offset;
			return fcbArray[fd].totalBytesRead; 
		}

		// if we are here then the total is less than the chunk size and 
		// we do not need to move to a new block to set the offset in another
		// block
		fcbArray[fd].byteInBlock += offset;
		fcbArray[fd].totalBytesRead += offset;
		fcbArray[fd].totalBytesWritten += offset;
		return fcbArray[fd].totalBytesRead; 
	} //SEEK_CUR

	if(whence & SEEK_END){
		// set the file position from the end + offset bytes
		// checking to see if the block we are currently on is the last block
		int nextBlock = getNextBlock(fcbArray[fd].blockOffset);
		int endBlock = getEOF(fcbArray[fd].DE);
		if(nextBlock != -2){
			// we are not currenlty in the last block of the file so 
			// we need to update the blockoffset to the last block 
			fcbArray[fd].blockOffset = endBlock;
			fcbArray[fd].wCurrentBlock = endBlock;
		}

		// we are currently at the last block
		// for reading, this will trigger a new block to be read
		fcbArray[fd].unReadBytes = 0;  
		// for writing, will trigger start of new block 
		fcbArray[fd].curBufPos = B_CHUNK_SIZE; 
		//position byteInBlock to the correct position 
		int overflow = offset - fcbArray[fd].unReadBytes;

		if(overflow > B_CHUNK_SIZE){
			// min amount of times we need to call getNextBlock
			// to put position pointer to offset
			int minBlock = (overflow + (B_CHUNK_SIZE-1))/B_CHUNK_SIZE;
			int remainder = overflow % B_CHUNK_SIZE;
			for(int i = 0; i < minBlock; i++){
				int nextBlock = getNextBlock(fcbArray[fd].blockOffset);
				if(nextBlock == -2){
					nextBlock = appendBlocks(fcbArray[fd].DE,2);
				}
				fcbArray[fd].blockOffset = nextBlock;
				fcbArray[fd].wCurrentBlock = nextBlock;
			}
			fcbArray[fd].byteInBlock = remainder;
			fcbArray[fd].totalBytesRead += offset;
			fcbArray[fd].totalBytesWritten += offset; 
		}
		// where to start reading or writing from in the block 
		fcbArray[fd].byteInBlock += (overflow % B_CHUNK_SIZE);
		return fcbArray[fd].totalBytesWritten;
	} // Seek End

	if(whence & SEEK_SET){
		// set the file position from the begining + offset bytes 
		// we do this by getting the location field of the DE and setting the 
		// current block to the start block 
		fcbArray[fd].blockOffset = fcbArray[fd].DE->location;
		fcbArray[fd].wCurrentBlock = fcbArray[fd].DE->location;

		if(offset > B_CHUNK_SIZE){
			int minBlock = (offset+ (B_CHUNK_SIZE-1))/B_CHUNK_SIZE;
			int remainder = offset % B_CHUNK_SIZE;
			for(int i = 0; i < minBlock; i++){
				int nextBlock = getNextBlock(fcbArray[fd].blockOffset);
				if(nextBlock == -2){
					nextBlock = appendBlocks(fcbArray[fd].DE,2);
				}
				fcbArray[fd].blockOffset = nextBlock;
				fcbArray[fd].wCurrentBlock = nextBlock;
			}
			fcbArray[fd].byteInBlock = remainder;
			fcbArray[fd].totalBytesRead += offset;
			fcbArray[fd].totalBytesWritten += offset; 
			// where to start reading or writing from in the block 
			fcbArray[fd].byteInBlock += (offset % B_CHUNK_SIZE);
			return fcbArray[fd].totalBytesWritten;
		}

		// the offset is not greater than our buffer (512 bytes)
		fcbArray[fd].byteInBlock = offset;
		fcbArray[fd].totalBytesRead += offset;
		fcbArray[fd].totalBytesWritten += offset; 
		// where to start reading or writing from in the block 
		return fcbArray[fd].totalBytesWritten;

	}// seek set
		
	return (0);
}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	if(fcbArray[fd].ROnlyIndic == 1){
		printf("This file is a read only file\n");
		return -1;
	}

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
	//Checks to see if the directory entry exist or not
	if(fcbArray[fd].DE == NULL)
		{
			return -1;
		}

	//Catch case if the user ask for more bytes than there are in the file
	if(count > fcbArray[fd].DE->size)
		{
			count = fcbArray[fd].DE->size;
		}
		
		int bytesLeftToWrite = count; //for this particular request to write
		int bytesWritten = 0; // for this PARTICULAR Request 

		while(bytesWritten <= count){

			// handles case where we need to clean our buffer, meaning that
			// we need to get the location of the next block we are writing to
			if(fcbArray[fd].curBufPos == 512){
					int nextBlock = getNextBlock(fcbArray[fd].wCurrentBlock);
					// if we are at the EOF block then we need to append more
					// blocks for the file
					if(nextBlock == -2){
						nextBlock = appendBlocks(fcbArray[fd].DE,2);
					}
					// update the fields given we are in a clean block/buffer
					fcbArray[fd].wCurrentBlock = nextBlock;
					fcbArray[fd].curBufPos = 0;
					fcbArray[fd].bytesLeftInBuffer = B_CHUNK_SIZE;
			}

			// we have enough space to put all of the users buffer into 
			// OUR buffer (so the block we are currently in) 
			if(bytesLeftToWrite < fcbArray[fd].bytesLeftInBuffer){
				memcpy(fcbArray[fd].writeBuf + fcbArray[fd].curBufPos, 
				buffer+fcbArray[fd].userBufPos, bytesLeftToWrite);

				fcbArray[fd].curBufPos += bytesLeftToWrite;
				fcbArray[fd].bytesLeftInBuffer = B_CHUNK_SIZE - fcbArray[fd].curBufPos;
				// 0 bc we are putting everything in the usersBuf into ourBuf 
				fcbArray[fd].userBufPos = 0; 
				fcbArray[fd].totalBytesWritten += bytesLeftToWrite;
				bytesWritten += bytesLeftToWrite;
				LBAwrite(fcbArray[fd].writeBuf, 1, fcbArray[fd].wCurrentBlock);
				fcbArray[fd].fileSize += bytesLeftToWrite;
				bytesLeftToWrite = 0; // we memcopied everything to our buffer
				return bytesWritten;
			}

			// works for both cases where we have stuff in buffer or a brand 
			// new buffer bc the first if statement in the loop
			if(bytesLeftToWrite > fcbArray[fd].bytesLeftInBuffer){
				// memcopy an amount of bytesLeftInBuffer to our  
				memcpy(fcbArray[fd].writeBuf + fcbArray[fd].curBufPos, 
				buffer+fcbArray[fd].userBufPos, fcbArray[fd].bytesLeftInBuffer);

				fcbArray[fd].curBufPos += fcbArray[fd].bytesLeftInBuffer; 
				fcbArray[fd].bytesLeftInBuffer = B_CHUNK_SIZE - fcbArray[fd].curBufPos;  
				// move our position in the users buffer to where we would start to get 
				// bytes from again to fill our own buffer 
				fcbArray[fd].userBufPos += fcbArray[fd].bytesLeftInBuffer; 
				fcbArray[fd].totalBytesWritten += fcbArray[fd].bytesLeftInBuffer;
				// update BYTESWRITTEN to how much we were able to fit into our buffer
				// we will go over the loop again 
				bytesWritten += fcbArray[fd].bytesLeftInBuffer;
				// we memcopied everything to our buffer
				bytesLeftToWrite = bytesLeftToWrite - fcbArray[fd].bytesLeftInBuffer; 
				LBAwrite(fcbArray[fd].writeBuf, 1, fcbArray[fd].wCurrentBlock);
				fcbArray[fd].fileSize += fcbArray[fd].bytesLeftInBuffer;
			}

		}
		
		return bytesWritten;
	}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}

	//Checks to see if the directory entry exist or not
	if(fcbArray[fd].DE == NULL)
		{
			return -1;
		}

	//Catch case if the user ask for more bytes than there are in the file
	if(count > fcbArray[fd].fileSize)
		{
			//printf("b_read() count > fcbArray[fd].fileSize\n");
			//printf("count: %d\n", count);
			//printf("fcbArray[fd].fileSize: %d\n",fcbArray[fd].fileSize);
			count = fcbArray[fd].fileSize;
		}

	//Catch case if what we're reading is more than what's leftover, so we don't go
	//over the file size
	//printf("COUNT: %d\n", count);
	if(count + fcbArray[fd].totalBytesRead > fcbArray[fd].fileSize)
		{
			//printf("b_read() count + fcbArray[fd].totalBytesRead > fcbArray[fd].totalBytesRead\n");
			count = fcbArray[fd].fileSize - fcbArray[fd].totalBytesRead;
			//printf("count: %d\n", count);
		}

	/**
	 * Local variables used to keep track of how much of each request we
	 * have processed, and used to terminate the while loop.
	 * Also used to move the writing place in the user's buffer
	*/
	int bytesRead = 0;
	int bytesLeftToRead = count;
	//printf("User buffer in b_read(): %s\n", buffer);
	//printf("fcbArray[fd].byteInBlock should be 199, it is: %d\n", fcbArray[fd].byteInBlock);
	while(bytesRead <= count)
		{
			//Catch case if we managed to reach the end of file
			//printf("Bytes left to read: %d\n", bytesLeftToRead);
			//printf("fcbArray[fd].unReadBytes: %d\n", fcbArray[fd].unReadBytes);
			if(fcbArray[fd].totalBytesRead >= fcbArray[fd].fileSize) //fcbArray[fd].DE->fileSize)
				{
					signed int dif = fcbArray[fd].totalBytesRead - fcbArray[fd].DE->fileSize;
					printf("Exiting while loop in b_read\n");
					return 0;
				}
		
		//Catch case where our buffer is empty, but we need more than just B_CHUNK_SIZE to read
		if(bytesLeftToRead > B_CHUNK_SIZE && fcbArray[fd].unReadBytes == 0)
			{
	
				//Minimum of amount of blocks we need to process request
				int ceiling = (bytesLeftToRead + (B_CHUNK_SIZE-1))/B_CHUNK_SIZE;
				int remainder = bytesLeftToRead % B_CHUNK_SIZE;
				
				//Catch case where we read one or more blocks into the user's buffer
				if(remainder == 0 && ceiling > 1)
					{

						// since this is a FAT, if we need to move to the next
						// block in the chain we need to call getNextBlock
						// the for loop will traverse through the chain
						// to get us the block we need to read 
						for(int i = 0; i < ceiling; i++){
							LBAread(buffer+bytesRead, 1, fcbArray[fd].blockOffset);
							bytesRead += B_CHUNK_SIZE;
							bytesLeftToRead = bytesLeftToRead - B_CHUNK_SIZE;
							int nextBlock = getNextBlock(fcbArray[fd].wCurrentBlock);
							if(nextBlock == -2){
								nextBlock = appendBlocks(fcbArray[fd].DE,2);
							}
							fcbArray[fd].blockOffset = nextBlock;
						}
		
						fcbArray[fd].byteInBlock = B_CHUNK_SIZE;
						fcbArray[fd].totalBytesRead += ceiling*B_CHUNK_SIZE;
						fcbArray[fd].unReadBytes = 0;
						return bytesRead;
					}
				/**
				 * Catch case for where we have a remainder, but our buffer is empty,
				 * So we need to read a total of Ceiling - 1 Blocks into the user's buffer
				 * We will go through the loop again to handle the remainder
				*/
				else
				{
					LBAread(buffer+bytesRead, 1, fcbArray[fd].blockOffset);
					bytesRead += B_CHUNK_SIZE;
					bytesLeftToRead -= B_CHUNK_SIZE;
					
					int nextBlock = getNextBlock(fcbArray[fd].wCurrentBlock);
					if(nextBlock == -2){
						nextBlock = appendBlocks(fcbArray[fd].DE,2);
					}
					fcbArray[fd].blockOffset = nextBlock;

					fcbArray[fd].byteInBlock = B_CHUNK_SIZE;
					fcbArray[fd].totalBytesRead += B_CHUNK_SIZE;
					fcbArray[fd].unReadBytes = 0; //in the buffer
				}
			}// if

			else
			{
				//printf("Bytes left to read is less than chunk size\n");
				if(bytesLeftToRead < B_CHUNK_SIZE)
				{
					//CASE 1: The buffer is empty and unread bytes = 0
					if(fcbArray[fd].unReadBytes == 0)
					{
						//printf("\nCASE 1: The buffer is empty and unread bytes = 0");
						//printf("Bytes left to read: %d\n", bytesLeftToRead);
						LBAread(fcbArray[fd].buf, 1, fcbArray[fd].blockOffset);
						// this needs to be replaced by a call to getNextBlock()
						//printf("Reading Block: %s\n", fcbArray[fd].buf);

						int nextBlock = getNextBlock(fcbArray[fd].wCurrentBlock);
						if(nextBlock == -2){
							//printf("nextBlock is EndOfFile, callinf appendBlocks\n");
							nextBlock = appendBlocks(fcbArray[fd].DE,2);
						}
						fcbArray[fd].blockOffset = nextBlock;

						memcpy(buffer+bytesRead,fcbArray[fd].buf+fcbArray[fd].byteInBlock,bytesLeftToRead);

						bytesRead += bytesLeftToRead;
						fcbArray[fd].byteInBlock = 0 + bytesLeftToRead;
						fcbArray[fd].totalBytesRead += bytesLeftToRead;
						fcbArray[fd].unReadBytes = B_CHUNK_SIZE - bytesLeftToRead;
						//printf("bytesRead: %d\n", bytesRead);
						return bytesRead;
					}
					//CASE 2: We have unread bytes in our buffer
					else
					{
						//printf("\nCASE 2: We have unread bytes in our buffer");
						//Catch case where the user's count will be more than our leftover buffer space
						if(bytesLeftToRead > fcbArray[fd].unReadBytes)
						{
							//printf("100\n");
							//Used to empty the buffer and then read in the remaining bytes
							memcpy(buffer+bytesRead, fcbArray[fd].buf+fcbArray[fd].byteInBlock, fcbArray[fd].unReadBytes);
							bytesRead += fcbArray[fd].unReadBytes;
							bytesLeftToRead = bytesLeftToRead - fcbArray[fd].unReadBytes;
							fcbArray[fd].byteInBlock = B_CHUNK_SIZE;
							fcbArray[fd].totalBytesRead += fcbArray[fd].unReadBytes;
							fcbArray[fd].unReadBytes = 0;
						}
						//Catch case where count is enough to fit in our leftover buffer space
						else
						{
							//printf("200\n");
							//printf("\nusers buffer b4: \n%s\n", buffer);
							//printf("bytesRead: %d\n", bytesRead);
							memcpy(buffer+bytesRead, fcbArray[fd].buf+fcbArray[fd].byteInBlock, bytesLeftToRead);
							//printf("users buffer after: \n%s\n", buffer);
							bytesRead += bytesLeftToRead;
							fcbArray[fd].byteInBlock += bytesLeftToRead;
							fcbArray[fd].totalBytesRead += bytesLeftToRead;
							fcbArray[fd].unReadBytes = B_CHUNK_SIZE - fcbArray[fd].byteInBlock;
							bytesLeftToRead = 0; 
							return bytesRead;
						}
					}
				}
				/**
				 * Case where the left over amount of bytes to read is greater than B_CHUNK_SIZE
				 * and that we still have unread amount bytes leftover in our buffer
				*/
				else
				{
					memcpy(buffer + bytesRead, fcbArray[fd].buf + fcbArray[fd].byteInBlock, fcbArray[fd].unReadBytes);
					bytesRead+= fcbArray[fd].unReadBytes;
					bytesLeftToRead -= fcbArray[fd].unReadBytes;
					fcbArray[fd].byteInBlock = B_CHUNK_SIZE;
					fcbArray[fd].totalBytesRead += fcbArray[fd].unReadBytes;
					fcbArray[fd].unReadBytes = 0;
				}
			} // outer else
			printf("bytesRead: %d\n", bytesRead);
		}

		return bytesRead;
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{
		fcbArray[fd].DE->fileSize = fcbArray[fd].fileSize;
		writeDir(fcbArray[fd].ppi->parent);

		free(fcbArray[fd].buf); //Deallocates once we close everything to free space in memory
		free(fcbArray[fd].ppi);
		// free ppi from the open
		fcbArray[fd].buf = NULL;
		fcbArray[fd].fileName[0] = '\0';
		fcbArray[fd].fileSize = 0;
		fcbArray[fd].DE = NULL;
		fcbArray[fd].byteInBlock = 0;
		fcbArray[fd].blockOffset = 0;
		fcbArray[fd].bytesLeftInBuffer = 0;
		fcbArray[fd].curBufPos = 0;
		fcbArray[fd].index = 0;
		
		// for(int i = 0; i < MAXFCBS; i++)
		// 	{
		// 		fcbArray[fd].fileName[0] = '\0';
		// 		fcbArray[fd].fileSize = 0;
		// 		fcbArray[fd].DE = NULL;
		// 	}
	}
