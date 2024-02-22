/**************************************************************
* Class:  CSC-415-03 Fall 2023
* Names: Angelo, Bryan, Natalie, Precious, Westley
* Student IDs: 921008618, 916956188, 920698945, 921582542, 922841683 
* GitHub Name: westleyc30
* Group Name: Blob
* Project: Basic File System
*
* File: mfs.c
*
* Description: This is used to help create any functionality
*              for our file system interface, so the user
*              can be able to interact with any of the given
*              functions for the file system. We also merged our
*              own functions into one file after we have worked on them
*              in separate files for testing purposes and ease of access
*
**************************************************************/
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "b_io.h"
#include "mfs.h"
#include "parsePath.h"
#include "directories.h"
#include "FATInit.h"

/**
 * Made global variables that will help keep track
 * of what our root directory is, and where we are
 * in our current directory. This is used to help
 * traverse in parsePath until we find our destination
*/
directoryEntry* rootDirParse;
directoryEntry* currDirParse;

//This is for the file path for the cwd we are in
char * cwd = "/";

/**
 * Two helper functions that help set both of the root
 * and current directory, both of which are needed
 * durring the intial start up of the file system,
 * mainly for the . and ..
*/
void setRootDirParse(directoryEntry* testRoot){
   rootDirParse = testRoot;
}

void setCurrDirParse(directoryEntry* testDir) {
   currDirParse = testDir;
}

/**
 * Helper function that helps to indicate
 * if the entry for directoryEntry is a
 * directory or not
*/
int isADirectory(directoryEntry *parent)
{
   if (parent -> isDir == 1) {
      return 1;
   } else {
      printf("Not a directory\n");
      return 0;
   }
   
}

/**
 * Helper function that helps to find
 * an existing entry in a given
 * directory
*/
int findEntryInDir(directoryEntry *parent, char *name)
{
   /**
    * Catch cases for whether or not the directory entry object 
    * or name of entry is valid or not
   */
   if (parent == NULL)
   {
      return -1;
   }

   if (name == NULL)
   {
      return -1;
   }

   //Used to go through all of the given current amount of directories
   int maxEntries = ((parent -> size)/sizeof(directoryEntry));

   for (int i = 0; i < maxEntries; i++)
   {
      int entryIndic = strcmp(parent[i].name, name);

      if (entryIndic == 0)
      {
         return i;
      }

      // return -1; // If no entry was found in the given directory
   }

   return -1;
}

int parsePath(char * path, ppInfo * ppi)
   {
   /**
     * Two if statements that act as flags and are needed
     * to stop function execution if either parsePath info
     * or the path is invalid
     */
     if(path == NULL)
         {
            return -1;
         }

      if (ppi == NULL)
      {
         return -1;
      }

      /**
       * Made a directory entry variable that will hold 
       * the starting directory
      */
      directoryEntry * startDir;

      if(path[0] == '/') //Checks to see if path is absolute
         {
            startDir = rootDirParse;
         }
      else //Checks to see if the path is relative
         {
            startDir = currDirParse;
         }
      
      /**
       * Since we know if the path is either absolute
       * (meaning it starts at '/') or if it's either
       * relative, that will make the start of that given
       * path our parent directory
      */
      directoryEntry * parent = startDir;

      /**
       * Using strtok in order to traverse through
       * the string that represents our given path
      */
      char * savePtr = NULL;
      char * token1 = strtok_r(path,  "/", &savePtr);

      /**
       * Checker for whether or not if the only path given
       * is /, then we know our current parsePath is just root
      */
      if(token1 == NULL)
         {
            if(strcmp(path, "/") == 0)
               {
                  ppi -> parent = parent;
                  ppi -> index = 0; //Because there is no element at all, the index wouldn't exist
                  ppi -> lastElement = NULL; //Since there is no last element, just the '/'
                  return 0;
               }
               return -1;
         }
   //Tokenization is used to traverse through the string of the path
   while (token1 != NULL) 
   {
      //Called a helper function to grab an existing directory
      int index = findEntryInDir(parent, token1);
      char *token2 = strtok_r(NULL, "/", &savePtr);
      if (token2 == NULL) //Indicates we hit the last directory entry
      {
         //Set all the values in ppi to the requested entry
         ppi->parent = parent;
         ppi->index = index;
         ppi->lastElement = strdup(token1);
         return 0;
      }

      if (index == -1)
      {
         return -2; // File or Path DNE
      }

      if (!isADirectory(&(parent[index])))
      {
         return -2; //Indicates the given item is not a directory
      }

      /**
       * Used to carry over the latest directory and
       * set it to the parent variable
      */
      directoryEntry *temp = loadDirectory(&parent[index]);

      if (temp == NULL)
      {
         return -3;
      }

      if (parent != startDir)
      {
         free(parent); // Cause you don't want to leave previous parentDirs in ram unless if it's a global variable
      }

      /**
       * Sets the parent entry to the latest entry from temp
       * Sets token1 to the latest '/' char until we hit the 
       * end of the path
      */
      parent = temp;
      token1 = token2;
   }

   return 0;
}

//---------------------------------------------------
//Main functions-------------------------------------

//Angelo's functions --------------------------------

struct fs_diriteminfo * diInfo;
//Instantiated the directoryInfo

char * fs_getcwd(char *pathname, size_t size)
//This will help set the current working directory
    {
       /**
        * currDir would be used as a global variable for
        * both set and get cwd, because it will help keep track of 
        * where we are currently located within our directory
       */
      strncpy(pathname, cwd, size);

      return pathname;
    }


/**
 * This will find the given path, and
 * initialize each of the fdDir struct members
*/
fdDir * fs_opendir(const char *path)
   {
      int len = strlen(path);
      char * pathname = malloc(sizeof(len));
      strcpy(pathname, path);
      // Used to help store and keep track of the current directory
      fdDir * fdd; 
      // Used to help store and keep track of the current path 
      ppInfo * ppi; 
      //Gave space to ppi so we can store our info for the path
      ppi = malloc(sizeof(ppInfo)); 
      parsePath(pathname, ppi); 
   
      if(ppi->index == -1) 
      //Check to see if ppi doesn't equal an invalid path index
         {  
            printf("Path is invalid\n");
            return NULL;
         }
      if(isADirectory(&(ppi->parent[ppi->index])))
      //Checks to see if ppi is a directory 
      //and sets the info for current directory opened
         {
            fdd = malloc(sizeof(fdDir)); 
            fdd->directory = loadDirectory(&(ppi->parent[ppi->index]));
            fdd->di = malloc(sizeof(diInfo));
            fdd->dirEntryPosition = 0;
            fdd->maxEntries = fdd->directory->size /sizeof(directoryEntry);
         }
      return fdd;
   }

//Used to free the current open directory
int fs_closedir(fdDir *dirp)
   {
      if(dirp != NULL){
         if(dirp->di != NULL){
            free(dirp->di);
         }

         free(dirp);
         return 0;
      }
      //the other case would be if dirp == NULL then we have nothing to free
      return 0;
   }

//Westley's functions -------------------------------
//Used to help indicate if the given item is a file
int fs_isFile(char *filename) {
  ppInfo* ppi = malloc(sizeof(ppInfo));
  int ret = parsePath(filename, ppi);
   //Catch case if parsePath didn't traverse and find the file sucessfully
  if(ret == -1){
     printf("ParsePath was unsuccesful in finding the file\n");
     return -1;
  }

   //Catch case if the file did not exist from parsePath
  if(ppi->index == -1){
     printf("File does not exist in this path\n");
     return -1;
  }

  directoryEntry * DE = &(ppi->parent[ppi->index]);
  //Another catch case if the user request a directory instead of a file
  if (isADirectory(DE) == 1) {
    printf("%s is a directory not a file.\n", filename);
    return 0;
  }

  /**
   * Used to free up any memory taken up by 
   * ppi since we already checked its info
  */
  free(ppi);

  //Return 1 to help indicate we found the requested file
  return 1;
}

//Natalie's functions -------------------------------
char * cleanPath(char * path)
    {
        
        char * str = path; // pointer to orginal path
        char * savePtr; // for str_tok to maintain context between calls
        // to store cleaned path parts 
        char ** cleanPathArray = malloc(strlen(path)* sizeof(char*));
        // indexer for clean path array
        int n = 0; 
        char * token;

         // add the beginging slash to the cleanPath array 
         if(path[0] == '/'){
               cleanPathArray[n] = "/";
               n++;
         }

         int i = 0;
        // tokenize using '/' as the delimiter 
        while((token = strtok_r(str, "/", &savePtr)) && i != 4){
            // if we are told to move up a directory then 
            // we need to adjust n accordingly so that when we
            // iterate through array to create the new path we stop at the 
            // correct index in the array 
            if(strcmp(token, "..") == 0){
                if(n != 0){
                    n--;
                    //printf("3 token: %s\n", token);
                    continue;
                }
            }
            // go throught the loop again do not change n
            if(*token == '.'){
                printf("2 token: %s\n", token);
                continue;
            }else{
               // add token to the cleaned path
               cleanPathArray[n] = token;
               n++;
               i++;
            }
            str = NULL;
        }

        // calculate the size of the final clean path 
        int size = 0;
        for(int i = 0; i < n; i++){
            size += strlen(cleanPathArray[i]);
            if(n != n-1){
                size++;
            }
        }

        // allocate memory for the final clean path
        char * retString = malloc(size+1);
        retString[0] = '\0';

        //reconstruct the clean path
        for(int i = 0; i < n; i++){
            strcat(retString, cleanPathArray[i]);
            if(i != n-1 && i != 0){
               strcat(retString, "/");
            }
        }
        return retString;

    }

char * concat(char* str1, char*str2){
    char * result = malloc(strlen(str1)+ strlen(str2) + 1);
    if(result == NULL){
        printf("Malloc did not work for concat()\n");
        return NULL;
    }
    strcpy(result, str1);
    strcpy(result, str2);

    return result;
}

int fs_setcwd(char *pathname)
{
   //Variable that helps to indicate if the working directory is relative
   int relFlag = 0;

   if (pathname == NULL || strcmp(pathname, "/") == 0) {
      // lets call parse path to get ppinfo struct for root
      ppInfo *ppi = malloc(sizeof(ppInfo));
      int ret = parsePath("/", ppi);
      if (ret == -1)
      {
         printf("Something wrong in fs_setcwd()\n");
         return -1;
      }

      // load the root dir and set it as the currrent directory 
      directoryEntry *result = loadDirectory(ppi->parent);
      setCurrDirParse(result);
      cwd = "/";
      free(ppi);
      return 0; // success
   }

   char *newPath;
   char *slash = "/";

   // absolute path case, if theres a slash
   if (pathname[0] == '/')
   {
      newPath = pathname;
   }
   
   // relative path case, no slash
   else
   {
      relFlag = 1;
      // check if the current dir is the root dir
      if (currDirParse[0].location == rootDirParse[0].location)
      {
         // allocate memory for the new path
         if ((newPath = malloc(strlen(cwd) + strlen(pathname) + 1)) != NULL)
         {
            newPath[0] = '\0'; // makes sure the memory is an empty string
            strcat(newPath, cwd);
            strcat(newPath, pathname);
         }
         else
         {
            printf("There was an error finding the path in setcwd\n");
            return -1;
         }
      }
      else
      {
         if ((newPath = malloc(strlen(cwd) + strlen(pathname) + 2)) != NULL)
         {
            newPath[0] = '\0'; // makes sure the memory is an empty string
            strcat(newPath, cwd);
            strcat(newPath, slash);
            strcat(newPath, pathname);
         }
         else
         {
            printf("There was an error finding the path in setcwd\n");
            return -1;
         }
      }
   } //else

   // create temp copy of the new path
   char *temp = malloc(strlen(newPath) );
   strcpy(temp,newPath);

   ppInfo *pp = malloc(sizeof(ppInfo));
   int ret = parsePath(temp, pp);

   // in case of invalid path return -1
   if (ret == -2 || ret == -3)
   {
      free(pp);
      free(temp);
      if(relFlag == 1){
         free(newPath);
      }  
      return -1;
   }

   // the directory does not exist/last element in path DNE
   if (pp->index == -1)
   {
      free(pp);
      free(temp);
      if(relFlag == 1){
         free(newPath);
      }   
      return -1;
   }

   // clean the path because right now it is not easily readable 
   char * clean = cleanPath(newPath);
   cwd = clean; //newPath;

   // we also need to set the global variable that holds the DE for 
   // the cur working dir 
   directoryEntry *result = loadDirectory( &(pp -> parent[pp ->index]) );
   setCurrDirParse(result);
   result == NULL;
   free(pp);
   pp = NULL;
   free(temp);
   temp = NULL;

   if(relFlag == 1){
      free(newPath);
   }

   newPath = NULL;
   return 0;
   // It takes in the int starting block of the directory you wanna load
   // and returns a directoryEntry*
   // which is the directory you're trying to load
}

int fs_stat(const char *path, struct fs_stat *buf)
   {
      char * pathname;
      strcpy(pathname, path);
      ppInfo * pp = malloc(sizeof(ppInfo));
      int ret = parsePath(pathname, pp);

      // in case of invalid path return -1
      if(ret == -2 || ret == -3){
         return -1;
      }

      if(pp->index == -1){
         return -1; // the directory does not exist/last element in path DNE
      }

      int pIndex = pp->index; //index of last element in path in the parent dir
      directoryEntry * dir = &(pp->parent[pIndex]);

      // fill in fs_stat struct
      buf->st_size = dir->size; 
      buf->st_blocks = (dir->size/BLOCK_SIZE) + 1;
      buf->st_blksize = BLOCK_SIZE * buf -> st_blocks;
      buf->st_accesstime = dir->accessed;
      buf->st_modtime = dir->modified;
      buf->st_createtime = dir->created;

      return 0; //success
   }

int fs_rmdir(const char *path) 
   {
      char * pathname;
      strcpy(pathname, path);
      ppInfo * pp = malloc(sizeof(ppInfo));
      int ret = parsePath(pathname,pp);

      // in case of invalid path return -1
      if(ret == -2 || ret == -3){
         return -1;
      }

      if(pp->index == -1){
         return -1; // the directory does not exist/last element in path DNE
      }

      // We now know the dir we want to remove exists
      int pIndex = pp->index; //index of last element in path in the parent dir
      directoryEntry * dir = &(pp->parent[pIndex]); 

      // 1 means it is a directory 
      if(fs_isDir(pathname) != 1){
         printf("Directory entry is not a directory\n");
         return -1;
      }

      //Now we now it exists and it is a dir
      freeBlocks(dir->location); 

      //Now we need to mark the dir as unused
      pp->parent[pIndex].name[0] = '\0';

      // rewrite all of pp->parent to disk again since we modified it 
      writeDir(pp->parent);
      return 0;

   }

int findFreeDirEntry(directoryEntry * parent)
   {
      if(parent == NULL)
      {
         return -1;
      }

      // getting limit for for loop
      int maxEntries = ((parent->size)/sizeof(directoryEntry));

      // go through the parent DE array and find the first empty DE signified 
      // by the its name = '\0'
      for(int i = 0; i < maxEntries; i++)
         {
               if(parent[i].name[0] == '\0'){
                  return i;
               }
         }
         return -1; //for no entries found
   }

int fs_mkdir(const char *pathname, mode_t mode)
   {

      char * path = malloc(sizeof(pathname));
      strcpy(path, pathname);

      // return values for parsePath
      ppInfo * pp = malloc(sizeof(ppInfo));
      int ret = parsePath(path, pp);

      // handling invalid paths
      if(ret == -2 || ret == -3){
         printf("Cannot make directory\n");
         return -1;
      }

      if(pp->index != -1){
         printf("Cannot make directory\n");
         return -1; //the case where the directory we want to make exists already
      }

      // we have checked that the path is valid and that we dont already have
      // a directory with the same name so now we can make one
      int index = findFreeDirEntry(pp->parent);
      directoryEntry * newDir = initDir(pp->parent);
      strncpy(pp->parent[index].name, pp->lastElement, 468); // changing the name from '/0' to lastElement
      
      // copying the "." entry 
      pp->parent[index].size = newDir[0].size;
      pp->parent[index].location = newDir[0].location;
      pp->parent[index].created = newDir[0].created;
      pp->parent[index].modified = newDir[0].modified;
      pp->parent[index].accessed = newDir[0].accessed;
      pp->parent[index].isDir = newDir[0].isDir;
      
      // rewrite all of pp->parent to disk again since we modified it 
      writeDir(pp->parent);
      free(path);
      return 0;
   }

char getType(directoryEntry * dir){
    char type; // f = file and d = directory 
    if(isADirectory(dir) == 0){
        type = 'd';
        return type;
    }
    type = 'f';
    return type;
}

// supports the ls function supposedly 
struct fs_diriteminfo *fs_readdir(fdDir *dirp)
    {   
        // fdDir contains the location of where we are in the directory array
        // dirp passed in gives you the fs_diriteminfo we want to return
        for(int i = dirp->dirEntryPosition; i < dirp->maxEntries; i++){
            if(dirp->directory[i].name[0] != '\0'){
                strcpy(dirp->di->d_name, dirp->directory[i].name);
                dirp->di->fileType = getType(&(dirp->directory[i]));
                
                // set the dirPos to the next entry in the DE array so we are 
                // ready for the next call to readDir
                dirp->dirEntryPosition = i+1; 
                return dirp->di;
            }
           
        }
        return NULL;
    }

//Precious's Functions ------------------------------
int fs_isDir(char * pathname){
   // use the parsePath to find the DE, which is just a blob
   ppInfo * ppi = malloc(sizeof(ppInfo));
   if (parsePath(pathname, ppi) == -1){
      printf("DE does not exist\n");
      return -1; // DE doesn't exist, exit with error
   }

   // check the contents of the directory entry
   if(ppi->parent[ppi->index].isDir == 0){ // need pointer to DE struct
      return 0;// this DE is a file
   }
   return 1; // passed all tests, this DE is a dir
}  

int fs_delete(char* filename) {
   //Used to store the path of file we're seeking
   ppInfo * ppi = malloc(sizeof(ppInfo)); 

   //Stores the location of the file once returned
   int ret = parsePath(filename, ppi);

   //Catch case if parsePath didn't traverse and find the file sucessfully
   if(ret == -1){
      printf("ParsePath was unsuccesful in finding the file\n");
      return -1;
   }

   //Catch case if the file did not exist from parsePath
   if(ppi->index == -1){
      printf("File does not exist in this path\n");
      return -1;
   }

   
   //Checks to see if the directoryEntry at the index is a file or not
   if( isADirectory(&(ppi->parent[ppi->index])) == 1 ) {
      printf("This is not file\n");
      return -1;
   }
   //We store the directory entry to the specific directory entry 
   //in the parent array that belongs to the file
   directoryEntry * fileLocation = &(ppi->parent[ppi->index]);

   //Here we store and return the starting block of our given file
   int result = freeBlocks(fileLocation->location); //returns 0 if sucessfull

   //Now we need to mark the dir as unused
   ppi->parent[ppi -> index].name[0] = '\0';

   // rewrite all of pp->parent to disk again since we modified it 
   writeDir(ppi->parent);

   free(ppi);
   return result;
}


//move file from src to path destination 
int fs_mvFile(char* srcPath, char* desPath) {
   char* tempSrcPath;
   int srcRelativeFlag = 0;
   char* tempDesPath;
   int desRelativeFlag = 0;
   char* slash = "/";

   // check if the src path is absolute or relative path and handle each case
   if(srcPath[0] == '/') {
      tempSrcPath = srcPath;
   } else {
      srcRelativeFlag = 1; // we need this in order free the tempSrcPath 
                           // variable correctly 
      // check if the current dir is the root dir 
      // in that case we construct the path differntly 
      if (currDirParse[0].location == rootDirParse[0].location)
      {
         if ( ( tempSrcPath = malloc(strlen(cwd) + strlen(srcPath) + 1) ) != NULL)
         {
            tempSrcPath[0] = '\0'; // makes sure the memory is an empty string
            strcat(tempSrcPath, cwd);
            strcat(tempSrcPath, srcPath);
         }
         else
         {
            printf("There was an error moving....\n");
            return -1;
         }
      }
      // handel creating the path for this case 
      else
      {
         if ( ( tempSrcPath = malloc(strlen(cwd) + strlen(srcPath) + 2) ) != NULL)
         {
            tempSrcPath[0] = '\0'; // makes sure the memory is an empty string
            strcat(tempSrcPath, cwd);
            strcat(tempSrcPath, slash);
            strcat(tempSrcPath, srcPath);
         }
         else
         {
            printf("There was an error moving....\n");
            return -1;
         }
      }
   }

   // use the temp path created above to call parsepath with 
   // this will give is the src path's DE 
   ppInfo* ppiSrc = malloc ( sizeof(ppInfo) );
   int retSrc = parsePath (tempSrcPath, ppiSrc);

   // check if the file exists in the current dir
   if( ppiSrc->index == -1 ) {
      printf("The file you want to move doesn't exist in the current directory.\n");
      if(srcRelativeFlag == 1) {
         free(tempSrcPath);
      }
      free(ppiSrc);
      return -1;
   }


   // check if the src path is a file 
   if( ppiSrc->parent[ppiSrc->index].isDir == 1 ) {
      printf("The entry you want to move is not a file.\n");
      if(srcRelativeFlag == 1) {
         free(tempSrcPath);
      }
      free(ppiSrc);
      return -1;
   }

   // doing the same process for the destination path 
   // first we check if the path is absolute or relative 
   if(desPath[0] == '/') {
      tempDesPath = desPath;
   } else {
      desRelativeFlag = 1; // used when freeing the dest path 
                           // only free this path if it is relative 

       // Check if the current directory is the root directory
      if (currDirParse[0].location == rootDirParse[0].location)
      {
         if ( ( tempDesPath = malloc(strlen(cwd) + strlen(desPath) + 1) ) != NULL)
         {
            tempDesPath[0] = '\0'; // makes sure the memory is an empty string
            strcat(tempDesPath, cwd);
            strcat(tempDesPath, desPath);
         }
         else
         {
            printf("There was an error moving....\n");
            return -1;
         }
      }

      // handle the relative path case 
      else
      {
         if ( ( tempDesPath = malloc(strlen(cwd) + strlen(desPath) + 2) ) != NULL)
         {
            tempDesPath[0] = '\0'; // makes sure the memory is an empty string
            strcat(tempDesPath, cwd);
            strcat(tempDesPath, slash);
            strcat(tempDesPath, desPath);
         }
         else
         {
            printf("There was an error moving....\n");
            return -1;
         }
      }
   }

   
   // Parse the destination path to obtain the destinations DE
   ppInfo* ppiDes = malloc ( sizeof(ppInfo) );
   int retDes = parsePath (tempDesPath, ppiDes);

   // Check if the destination directory exists
   if( ppiDes->index == -1 ) {
      printf("The directory you want to move to doesn't exist.\n");
      // for (int i = 0; i < ( ppiDes-> parent[0].size/512 ); i++)
      // {
      //    printf("Dir at %d is: %s\n",i ,ppiDes->parent[i].name);
      // }
      
      // manage memory accordingly 
      if(srcRelativeFlag == 1) {
         free(tempSrcPath);
      }

      free(ppiSrc);
      if(desRelativeFlag == 1) {
         free(tempDesPath);
      }
      
      free(ppiDes);
      return -1;
   }

   // Check if the destination is a directory and free memory accordingly
   if( ppiDes->parent[ppiDes->index].isDir != 1 ) {
      printf("The place you want to move to isn't a directory.\n");
      if(srcRelativeFlag == 1) {
         free(tempSrcPath);
      }
    
      free(ppiSrc);
      if(desRelativeFlag == 1) {
         free(tempDesPath);
      }
      
      free(ppiDes);
      return -1;
   }

   // Load the destination directory and find a free entry
   directoryEntry* loadDesDir = loadDirectory( &( ppiDes->parent[ppiDes->index] ) );
   int freeDesDirIndex = findFreeDirEntry(loadDesDir);
   
   // Copy the source entry to the destination directory
   loadDesDir[freeDesDirIndex] = ppiSrc->parent[ppiSrc->index];

   // strncpy( loadDesDir[freeDesDirIndex].name, ppiSrc->parent[ppiSrc->index].name, 472 );
   // loadDesDir[freeDesDirIndex].created = ppiSrc->parent[ppiSrc->index].created;
   // loadDesDir[freeDesDirIndex].isDir = ppiSrc->parent[ppiSrc->index].isDir;
   // loadDesDir[freeDesDirIndex].location = ppiSrc->parent[ppiSrc->index].location;
   // loadDesDir[freeDesDirIndex].size = ppiSrc->parent[ppiSrc->index].size;
   // loadDesDir[freeDesDirIndex].accessed = ppiSrc->parent[ppiSrc->index].accessed;
   // loadDesDir[freeDesDirIndex].modified = ppiSrc->parent[ppiSrc->index].modified;
   // loadDesDir[freeDesDirIndex]


   // Clear the name of the source entry
   ppiSrc->parent[ppiSrc->index].name[0] = '\0';

   // Write the updated source parent directory back to disk
   int writeSrcParent = writeDir(ppiSrc->parent);
   if(writeSrcParent == -1) {
      printf("Error writing srcParent back to disk...\n");
      return -1;
   }

   // Write the updated destination directory back to disk
   int writeDes = writeDir(loadDesDir);
   if(writeDes == -1) {
      printf("Error writing desDIr back to disk...\n");
      return -1; 
   }

    // Free allocated memory using the flags we set and return success
   if(srcRelativeFlag == 1) {
      free(tempSrcPath);
   }
   if(desRelativeFlag == 1) {
      free(tempDesPath);
   }
   free(loadDesDir);
   free(ppiSrc);
   free(ppiDes);

   return 0;

}