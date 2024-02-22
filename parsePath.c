/**************************************************************
 * Class:  CSC-415-03 Fall 2023
 * Names: Angelo, Bryan, Natalie, Precious, Westley
 * Student IDs: 921008618, 916956188, 920698945, 921582542, 922841683 
 * GitHub Name: westleyc30
 * Group Name: Blob
 * Project: Basic File System
 *
 * File: parsePath.c
 *
 * Description: This function acts as a helper when it comes to
 *              making new directories within other directories
 *              and allowing us to make a new path for every connected
 *              directory. It also contains a few other helper
 *              and mfs.c functions that would be moved later to mfs.c
 *
 **************************************************************/
// #include <fcntl.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <time.h>
// #include <unistd.h>
// #include "parsePath.h"
// #include "fsInit.h"

// /**
//  * Made global variables that will help keep track
//  * of what our root directory is, and where we are
//  * in our current directory. This is used to help
//  * traverse in parsePath until we find our destination
// */
// directoryEntry* rootDirParse;
// directoryEntry* currDirParse;


// // Helper functions-----------------------------------
// void setRootDirParse(directoryEntry* testRoot){
//    rootDirParse = testRoot;
// }

// void setCurrDirParse(directoryEntry* testDir) {
//    currDirParse = testDir;
// }

// //Test function to see if rootDir and currDir got set properly
// // void printRootDir(){
// //    rootDirParse = getRoot();
// //    printf("I made it into parsePath and my rootDir location should start at 307, it is:%d\n", rootDirParse -> location);
// // }

// // void printCurrDir(){
// //    currDirParse = getRoot();
// //    printf("I made it into parsePath and my currDir location should start at 307(Since we just started), it is:%d\n", currDirParse -> location);
// // }
// //Test ends here

// int isADirectory(directoryEntry *parent)
// {
//    if (parent -> isDir == 1) {
//       return 0;
//    } else {
//       return 1;
//    }
   
// }

// int findEntryInDir(directoryEntry *parent, char *name)
// {
//    if (parent == NULL)
//    {
//       return -1;
//    }

//    if (name == NULL)
//    {
//       return -1;
//    }

//    //Used to go through all of the given current amount of directories
//    int maxEntries = ((parent -> size)/sizeof(directoryEntry));

//    for (int i = 0; i < maxEntries; i++)
//    {
//       int entryIndic = strcmp(parent[i].name, name);

//       if (entryIndic == 0)
//       {
//          return i;
//       }

//       // return -1; // If no etry was found in the given directory
//    }

//    return -1;
// }

// // int fs_delete(char* filename) {
// //    //ppInfo* tempPpi = malloc( sizeof(ppInfo) );

// //    int indexOfFile = findEntryInDir(currDirParse, filename);
   
// //    //Checks to see if this directoryEntry even exist in the current directory
// //    if( indexOfFile == -1) {
// //       printf("This file doesn't exit in the current directory...\n");
// //       return -1;
// //    }

// //    //Checks to see if the directoryEntry at the index is a file or not
// //    if( isADirectory( &(currDirParse[indexOfFile]) ) == 0 ) {
// //       printf("This directory entry isn't a file... \n");
// //       return -1;
// //    }

// //    int result = freeBlocks(indexOfFile);

// //    return result;

// // }

// // //---------------------------------------------------
// // // Main-----------------------------------------------

// int parsePath(char * path, ppInfo * ppi)
//    {
//       /**
//        * Two if statements that act as flags and are needed
//        * to stop function execution if either parsePath info
//        * or the path is invalid
//       */
//       if(path == NULL)
//          {
//             return -1;
//          }

//       if (ppi == NULL)
//       {
//          return -1;
//       }

//       directoryEntry * startDir;

//       if(path[0] == '/') //Checks to see if path is absolute
//          {
//             startDir = rootDirParse;
//          }
//       else //Checks to see if the path is relative
//          {
//             startDir = currDirParse;
//          }
      
//       /**
//        * Since we know if the path is either absolute
//        * (meaning it starts at '/') or if it's either
//        * relative, that will make the start of that given
//        * path our parent directory
//       */
//       directoryEntry * parent = startDir;

//       /**
//        * Using strtok in order to traverse through
//        * the string that represents our given path
//       */
//       char * savePtr = NULL;
//       char * token1 = strtok_r(path,  "/", &savePtr);

//       /**
//        * Checker for whether or not if the only path given
//        * is /, then we know our current parsePath is just root
//       */
//       if(token1 == NULL)
//          {
//             if(strcmp(path, "/") == 0)
//                {
//                   ppi -> parent = parent;
//                   ppi -> index = -1; //Because there is no element at all, the index wouldn't exist
//                   ppi -> lastElement = NULL; //Since there is no last element, just the '/'
//                   return 0;
//                }
//                return -1;
//          }

//    while (token1 != NULL)
//    {
//       int index = findEntryInDir(parent, token1);
//       char *token2 = strtok_r(NULL, "/", &savePtr);
//       if (token2 == NULL)
//       {
//          ppi->parent = parent;
//          ppi->index = index;
//          ppi->lastElement = strdup(token1);
//          return 0;
//       }
//       if (index == -1)
//       {
//          return -2; // File or Path DNE
//       }
//       if (!isADirectory(&(parent[index])))
//       {
//          return -2;
//       }
//       directoryEntry *temp = loadDirectory(&parent[index]);
//       if (temp == NULL)
//       {
//          return -3;
//       }
//       if (parent != startDir)
//       {
//          free(parent); // Cause you don't want to leave previous parentDirs in ram unless if it's a global variable
//       }
//       parent = temp;
//       token1 = token2;
//    }

//    return 0;
// }