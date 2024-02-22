/**************************************************************
* Class:  CSC-415-03 Fall 2023
* Names: Precious
* Student IDs: 921582542
* GitHub Name: westleyc30
* Group Name: Blob
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"
#include "FATInit.h"
#include "directories.h"
#include "parsePath.h"
#define SIGNATURE 0xACE123456

typedef struct vcb {
	int blocks;	// amount of blocks
	int blockSize; // size of our blocks
	int freeBlocks; // amount of freeblocks
	int fatStart; // block that the FAT starts
	int freeSpaceStart; // block where the head of the free block chain is 
	int rootDirStart; // block the root dir starts on
	long signature; // signature to know this is our volume on system start
} vcb;

vcb* vcb_p; // global pointer to the vcb to modify values

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	/* TODO: Add any code you need to initialize your file system. */

	/* STEP 1: initializing the VCB */
	vcb_p = malloc(blockSize); // global pointer to the vcb to modify values
	
	/* STEP 2: initalizing FAT */
	vcb_p->blocks = numberOfBlocks;
	vcb_p->blockSize = blockSize;

	//An error handler if our FATInitialize function returns an error code
	int errorCheck = FATInitialize(numberOfBlocks, blockSize); 
	if(errorCheck != 0) {
		printf("FAT wasn't initialized\n");
		return -1;
	}
	
	vcb_p -> fatStart = getFATStart();

	/* STEP 3: initializing root directory */ 
	directoryEntry* rootDir = initDir(NULL);
	vcb_p->rootDirStart = rootDir[0].location;

	/**
	 * Called two helper functions from
	 * mfs.h in order to store the actual root
	 * and current directory ".." on start up
	 * for the file system
	*/
	setRootDirParse(rootDir);
	setCurrDirParse(rootDir);
	
	/**
	 * Since we're trying to store the head of our chain of free blocks
	 * where our free space would start for our file system,
	 * we would call getFreeSpaceStart since our freeSpaceStart
	 * member is modified through various helper functions from 
	 * FATInit.c
	*/
	vcb_p->freeSpaceStart = getFreeSpaceStart(); 

	/**
	 * Since we're trying to keep track of the actual number of 
	 * free blocks that appear in our file system,
	 * we would simply just subtract the total number of blocks
	 * our file system holds, but subtract it by how much our vcb,
	 * fat table, and root takes up just to get an accurate idea
	 * of how many total free blocks we actually have in our file
	 * system.
	*/
	vcb_p->freeBlocks = numberOfBlocks - vcb_p->freeSpaceStart; 

	/**
	 * Since any file within the file system is specifically
	 * formatted to our file system, we use a hex as a
	 * signature
	*/
	vcb_p->signature = SIGNATURE;


	// LBA write -> buffer: , count: 1 , position: 0
	LBAwrite(vcb_p, 1, 0);
	// check that LBAwrite returns 1 written block

	return 0;
	}


//Used to free up any resources and exit gracefully in our file system
void exitFileSystem ()
	{
	free(vcb_p);
	vcb_p = NULL;
	freeFATMap();
	printf ("System exiting\n");
	}