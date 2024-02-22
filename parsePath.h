/**************************************************************
* Class:  CSC-415-03 Fall 2023
* Names: Angelo, Bryan, Natalie, Precious, Westley
* Student IDs: 921008618, 916956188, 920698945, 921582542, 922841683 
* GitHub Name: westleyc30
* Group Name: Blob
* Project: Basic File System
*
* File: parsePath.h
*
* Description: This acts as a template for parsePath.c
*
**************************************************************/
#include "directories.h"
#include "FATInit.h"

#ifndef PARSE_PATH_H
#define PARSE_PATH_H

#define BLOCK_SIZE 512

/**
 * Structure that would store the needed
 * information for a parsed path, and updating
 * that information will help us traverse that
 * given path whenever parsepath is called
*/
typedef struct ppInfo {
    directoryEntry * parent; // pointer to last element's parent
    char * lastElement; // pointer to last element in path
    int index; // index of last element in parent DE array
} ppInfo;

/**
 * Helper function for detecting if the
 * directoryEntry object is a directory
 * or a file based on one of the given
 * members in directoryEntry
*/
int isADirectory(directoryEntry *parent);

//Used to help load a directoryEntry onto disk
directoryEntry * loadDir(directoryEntry * parent);

//Used to help find the entry (file or directory) within a directory
int findEntryInDir(directoryEntry * parent, char * name);

//Used to help create and connect a given path between various directories
//and store the information of that given path
int parsePath(char * path, ppInfo * ppi);

//Used to set up the root and current directory on start up for the file system
void setRootDirParse(directoryEntry* testRoot);
void setCurrDirParse(directoryEntry* testDir);

#endif