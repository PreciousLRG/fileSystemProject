/**************************************************************
 * Class:  CSC-415-03 Fall 2023
 * Names: Angelo, Natalie, Bryan
 * Student IDs: 921008618, 920698945, 916956188
 * GitHub Name: westleyc30
 * Group Name: Blob
 * Project: Basic File System
 *
 * File: FATInit.c
 *
 * Description: This file focuses on being able to initialize our
 *              fat table for the entire file system.
 *
 **************************************************************/

#include "FATInit.h"
#define BLOCK_SIZE 512

int FATByteSize;    // Used to help indicate the size for our map in bytes 
                    // based on the FAT system
int freeSpaceStart; // Used to help find the starting block of our free space
int FATSpaceStartingBlock; // Used to indicate where our FATMap beings
int blocksNeeded; // The size of our FATMap in blocks
int blocksOfEntireVolume; // The size of the entire volume in blocks 
                         //(Not just the FATMap)
int userBlockSize; // The user specified block size
int freeBlocksLeft;
directoryEntry *rootDir;

blockIndicator *FATMap;

// We can change this to return a pointer to the FAT so we don't need to 
// read over and over
int FATInitialize(uint64_t numOfBlocks, uint64_t blockSize)
{
    // numOfBlocks that is used to keep track of how many blocks we're 
    // starting out with
    blocksOfEntireVolume = numOfBlocks;
    userBlockSize = blockSize;

    // For default, this is 156248
    FATByteSize = numOfBlocks * sizeof(blockIndicator); 
    // Used to grab the actual FATSize in size of bytes
    // Each block is an index
    // so it will be the numOfBlocks * sizeof(blockIndicator) 
    // to get the actual size of the entire freeSpace map

    blocksNeeded = (FATByteSize + (userBlockSize - 1)) / userBlockSize; 

    // For default, this is now 156672
    FATByteSize = blocksNeeded * userBlockSize; 

    // allocaitng the memeory needed for FAT 
    FATMap = malloc(FATByteSize);

    // Made the total space for our free space map in memory
    // Ex: FATByteSize = 19531 * sizeof(blockIndicator) = 156248 bytes
    // For how many blocks this entire map will take up,
    // it will be ( 156248 bytes + ( 512 - 1 ) ) / 512 bytes = 306 total blocks

    // Finding where our freespace map starts should happen before we 
    // initialize the free indexes
    FATSpaceStartingBlock = 1;
    int LBABlocksWritten;

    while (FATSpaceStartingBlock < (numOfBlocks - blocksNeeded))
    {
        // We try writing blocksNeeded number of blocks to our disk, 
        // if it doesn't write the blocksNeeded,
        // we know our starting block is corrupted
        LBABlocksWritten = LBAwrite(FATMap, blocksNeeded, FATSpaceStartingBlock);

        if (LBABlocksWritten == blocksNeeded)
        {
            // This will constantly search for the block to start out when 
            // writing into disk
            //  printf("This block is corrupted, moving onto next block");
            for (int i = 0; i < blocksNeeded; i++)
            {
                // Set the blocks used by the FAT to used
                FATMap[FATSpaceStartingBlock + i].usedBlock = 1; 
            }
            break;
        }

        FATSpaceStartingBlock++;
    }

    if (FATSpaceStartingBlock >= (numOfBlocks - blocksNeeded))
    {
        printf("There was an issue writting the freespace map to disk...\n");
        free(FATMap);
        FATMap = NULL;
        return -1;
    }

    freeSpaceStart = (FATSpaceStartingBlock + blocksNeeded); // Where usable free space starts

    if (freeSpaceStart >= numOfBlocks)
    {
        printf("There is no freespace after writing the freespace map.\n");
        free(FATMap);
        FATMap = NULL;
        return -1;
    }

    // Set all blocks before the last block to point to the next block
    for (int i = FATSpaceStartingBlock; i < (numOfBlocks - 1); i++)
    {
        // This will initialize each free index until it hits the last one
        // Each index will be initialized to the next block
        FATMap[i].nextBlock = i + 1; 
    }

    // set last block of the volume to point to whatever indicates the end of 
    // volume. NULL for now. Can change as the block is used
    FATMap[numOfBlocks - 1].nextBlock = -2;

    for (int i = freeSpaceStart; i < numOfBlocks; i++)
    {
        FATMap[i].usedBlock = 0;
    }

    // LBAWrite one more time after we changing each block pointing to the 
    // next and changing used
    LBABlocksWritten = LBAwrite(FATMap, blocksNeeded, FATSpaceStartingBlock);

    if (LBABlocksWritten != blocksNeeded)
    {
        printf("There was an error writing to disk after trying to make changes...\n");
        free(FATMap);
        FATMap = NULL;
        return -1;
    }
    return 0;
}

// We'll change this to utilize a pointer to FAT later
int allocateRoot(uint64_t numOfBlocks)
{

    int returnStartBlock = freeSpaceStart;
    for (int i = 0; i < numOfBlocks; i++)
    {
        // we mark the blocks as used
        FATMap[freeSpaceStart + i].usedBlock = 1; 
    }

    // Makes the root's last block point to end of file
    FATMap[freeSpaceStart + (numOfBlocks - 1)].nextBlock = -2; 

    // writing the blocks FAT needs 
    if (LBAwrite(FATMap, blocksNeeded, FATSpaceStartingBlock) != blocksNeeded)
    {
        printf("Error writing the FAT back to disk...");
        free(FATMap);
        FATMap = NULL;
        return -1;
    }

    freeSpaceStart += numOfBlocks;
    freeBlocksLeft = blocksOfEntireVolume - freeSpaceStart;

    return returnStartBlock;
}

int getFATStart()
{
    return FATSpaceStartingBlock;
}

int getFreeSpaceStart()
{
    return freeSpaceStart;
}

// Returns the starting block index on success, -1 on failure
int allocateBlocks(uint64_t numOfBlocks)
{
    int returnStartBlock;
    int count = 0;
    int prevIndex;
    
    // is there enough space for the allocation
    if(numOfBlocks > freeBlocksLeft) {
        printf("Not enough free space left\n");
        return -1; 
    }

    // Iterate through the blocks of the entire volume to find free blocks 
    // for allocation
    for (int i = freeSpaceStart; i < blocksOfEntireVolume; i++)
    {
        // Check if the current block is not in use
        if (FATMap[i].usedBlock == 0)
        {
            // If this is the first block in the allocation, initialize variables
            if (count == 0)
            {
                returnStartBlock = i;
                prevIndex = i;
                FATMap[i].usedBlock = 1;
                count++;

            }
            else
            {
                FATMap[prevIndex].nextBlock = i;
                prevIndex = i;
                FATMap[i].usedBlock = 1;
                count++;
            }

            // Check if the required number of blocks have been allocated
            if (count == numOfBlocks)
            {
                FATMap[i].nextBlock = -2; // -2 for EOF for now
                 // Write the updated FATMap back to disk
                if (LBAwrite(FATMap, blocksNeeded, FATSpaceStartingBlock) != blocksNeeded)
                {
                    printf("Error writing the FAT back to disk...");
                    free(FATMap);
                    FATMap = NULL;
                    return -1;
                }
                // Update the count of free blocks and return the starting block index
                freeBlocksLeft = freeBlocksLeft - count;
                return returnStartBlock;
            }
        }
    }

    // If we reach this point, something went wrong
    free(FATMap);
    FATMap = NULL;
    return -1;
}


int freeBlocks(uint64_t startBlock)
{
    // tracking current block in chain to be freed 
    int currBlock = startBlock;
    // Iterate through the block chain until reaching the end marker (-2)
    while (currBlock != -2)
    {
        // Mark the current block as unused
        FATMap[currBlock].usedBlock = 0;
        // Move to the next block in the chain
        currBlock = FATMap[currBlock].nextBlock;
        // increase count of free blocks left
        freeBlocksLeft++;

    }

    // Write the updated FATMap back to disk after freeing the blocks
    if (LBAwrite(FATMap, blocksNeeded, FATSpaceStartingBlock) != blocksNeeded) {
        printf("Error writing the FAT back to disk after freeing blocks...\n");
        return -1;
    }
    return 0;
}

void freeFATMap()
{
    free(FATMap);
    FATMap = NULL;

    free(rootDir);
    rootDir = NULL;
}

int writeDir(directoryEntry *dir)
{
    // Start with the first block index of the directory
    int currIndex = dir[0].location;
    // Variable to track the current entry in the directoryEntry array
    int currEntryInArray = 0;

    // Iterate through the blocks of the directory
    while (currIndex != -2)
    {   
        // Write the current directory entry to the disk
        if (LBAwrite(&dir[currEntryInArray], 1, currIndex) != 1)
        {
            printf("Error writing to disk from writeDir...");
            free(dir);
            dir = NULL;
            return -1;
        }

        // Move to the next block in the directory
        currIndex = FATMap[currIndex].nextBlock;
        // Move to the next entry in the directoryEntry array
        currEntryInArray++;
    }

    return 0;
}

directoryEntry *getRoot()
{
    return rootDir;
}

void setRoot(directoryEntry *inputRootDir)
{
    rootDir = inputRootDir;
}

directoryEntry *loadDirectory(directoryEntry *parent)
{
    // Allocate memory for the new directory based on the size of the parent directory
    directoryEntry *returnDirectory = malloc(parent[0].size);
    int currIndex = 0; // Keeps track of which directoryEntry we're at
    // keeps track of which block to read from memory;
    int currBlock = parent[0].location; 
       
       // Iterate through the blocks of the directory 
       while (currBlock != -2)
       {
            // Read the current directory entry from disk into memory
           if (LBAread(&returnDirectory[currIndex], 1, currBlock) != 1)
           {
               printf("Error loading directory...\n");
               return NULL;
          }
          
          // Move to the next entry in the directoryEntry array
          currIndex++;
           // Move to the next block in the directory
          currBlock = FATMap[currBlock].nextBlock;
       }

    return returnDirectory;
}

int getEOF(directoryEntry* dirEntry) {

    // something is wrong with the location of this DE
    if(dirEntry->location < 0) {
        printf("There was an error getting the dirEntry start block.\n");
        return -1;
    }

    // Start with the location block of the directory entry
    int currBlock = dirEntry->location;
    int returnBlock;

    // Iterate through the blocks of the directory until reaching the end marker (-2)
    while(currBlock != -2) {
        // Update the returnBlock with the current block
        returnBlock = currBlock; 
        // Move to the next block in the directory
        currBlock = FATMap[currBlock].nextBlock;
    }
    
    return returnBlock;

}

// helper function for when we reach the end of the chain of blocks 
int appendBlocks(directoryEntry* dirEntry, int numOfBlocks) {
    int currEnd = getEOF(dirEntry);
    // allocate blocks will return the starting block of the chain
    // of blocks allcoateBlocks allocates, this is where the chain continues
    int newChainStart = allocateBlocks(numOfBlocks);

    if(newChainStart == -1) {
        printf("Something went wrong appending blocks.\n");
        return -1;
    }

    // "append" the new chain given by allocatBlocks to the current chain 
    FATMap[currEnd].nextBlock = newChainStart;
    return newChainStart;

}

int getNextBlock(int currBlock) {
    return FATMap[currBlock].nextBlock;
}
