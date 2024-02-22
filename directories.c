/**************************************************************
 * Class:  CSC-415-03 Fall 2023
 * Names: Bryan, Westley
 * Student IDs: 916956188, 922841683
 * GitHub Name: westleyc30
 * Group Name: Blob
 * Project: Basic File System
 *
 * File: directories.c
 *
 * Description: Basic File System - Key File I/O Operations
 *
 **************************************************************/
// #include <fcntl.h>
// #include <stdio.h>
// #include <stdlib.h> // for malloc
// #include <string.h> // for memcpy
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <time.h>
// #include <unistd.h>
// #include "FATInit.h"
#include "directories.h"

#define DEFAULT_ENTRIES 50
#define BLOCK_SIZE 512

directoryEntry* initDir(directoryEntry *parent)
{
    directoryEntry *p;

    // 14800 for default, 25600 for 512 directoryEntry size
    int bytesNeeded = DEFAULT_ENTRIES * sizeof(directoryEntry);
    // 29 Blocks for default, 50 for other       
    int blocksNeeded = (bytesNeeded + (BLOCK_SIZE - 1)) / BLOCK_SIZE; 
    // 14848 for default, 25600 other
    bytesNeeded = blocksNeeded * BLOCK_SIZE; 
    // 50 entries for default, 50 for other                         
    int actualDirEntries = bytesNeeded / sizeof(directoryEntry);      

    // bytes needed holds the amount of bytes we need to store the 
    // array of DEs this directory will have 
    directoryEntry *dir = malloc(bytesNeeded); // dirEntry array

    int startBlock;

    // from here we initialize all the memebers for the DEs fields
    for (int i = 0; i < actualDirEntries; i++)
    {
        dir[i].name[0] = '\0'; // set name to NULL;
    }

    if (parent == NULL) //For milestone 1
    {
        startBlock = allocateRoot(blocksNeeded); // Should be 307
    } else {
        startBlock = allocateBlocks(blocksNeeded); // Should be 307
    }

    strcpy(dir[0].name, ".");

    dir[0].location = startBlock; // set location = startBlock

    dir[0].size = actualDirEntries * sizeof(directoryEntry);

    time_t t = time(NULL);
    dir[0].created = t;

    dir[0].modified = t;

    dir[0].accessed = t;

    dir[0].isDir = 1;

    if (parent != NULL)
    {
        p = parent;
    }
    else
    {
        setRoot(dir);
        p = &dir[0]; // in the case of initializing root dir
    }

    strcpy(dir[1].name, "..");
    
    dir[1].location = p->location;
   
    dir[1].size = p->size;
    
    dir[1].created = p->created;
    
    dir[1].modified = p->modified;
    
    dir[1].accessed = p->accessed;

    dir[1].isDir = p->isDir;

    // writing the new directory to disk to save the changes 
    writeDir(dir);
    return dir;

    //return 0;
}
