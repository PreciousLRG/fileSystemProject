/**************************************************************
* Class:  CSC-415-03 Fall 2023
* Names: Bryan
* Student IDs:  916956188
* GitHub Name: westleyc30
* Group Name: Blob
* Project: Basic File System
*
* File: FATInit.h
*
* Description: Created predefined FAT initialization and manipulation functions
*
*
**************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"


// #ifndef uint64_t
// typedef u_int64_t uint64_t;
// #endif
// #ifndef uint32_t
// typedef u_int32_t uint32_t;
// #endif
// typedef unsigned long long ull_t;

//Find out a way to include uint64_t into our header files cause uint64_t is only defined in fsLow.h

#ifndef FAT_INIT_H
#define FAT_INIT_H

#include "directories.h"

// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <stdio.h>
// #include <string.h>

// #include "fsLow.h"
// #include "mfs.h"

typedef struct blockIndicator
{
    int nextBlock;
    int usedBlock; // 0 for unused, 1 for used
} blockIndicator;  // size is 8 bytes

int FATInitialize(u_int64_t numOfBlocks, u_int64_t blockSize);
int allocateBlocks(uint64_t numOfBlocks);
int freeBlocks(uint64_t startBlock);

int allocateRoot(uint64_t numOfBlocks);
int getFATStart();
int getFreeSpaceStart();
void freeFATMap();
int writeDir(directoryEntry* dir);
directoryEntry* loadDirectory(directoryEntry* parent);
directoryEntry* getRoot();
void setRoot(directoryEntry* inputRootDir);
//Do a writeFile function

int getEOF(directoryEntry* dirEntry);
int appendBlocks(directoryEntry* dirEntry, int numOfBlocks);
int getNextBlock(int currBlock);

#endif

// int deleteBlocks();
