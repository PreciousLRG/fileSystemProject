/**************************************************************
 * Class:  CSC-415-03 Fall 2023
 * Names: Bryan, Westley
 * Student IDs: 916956188, 922841683
 * GitHub Name: westleyc30
 * Group Name: Blob
 * Project: Basic File System
 *
 * File: directories.h
 *
 * Description: Created predefined directory function
 *
 **************************************************************/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h> // for malloc
#include <string.h> // for memcpy
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "FATInit.h"

#ifndef DIRECTORIES_H
#define DIRECTORIES_H

// Contents of fsLow.h
typedef struct directoryEntry{
  char name[468];// 472 for 512 size, 256 for 296
  int location; 
  unsigned int size; //size in bytes of directory
  unsigned int fileSize; //size in bytes for the file we're using
  time_t created;
  time_t modified;
  time_t accessed;
  int isDir;

} directoryEntry; // size is 296 bytes, 512 for 472

directoryEntry* initDir(directoryEntry * parent);
#endif