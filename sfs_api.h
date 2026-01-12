#ifndef SFS_API_H
#define SFS_API_H

/**
 * Initializes or loads the filesystem disk.
 * 'fresh' == 1 creates a fresh filesystem, otherwise loads existing.
 */
void mksfs(int fresh);

/**
 * Displays the content of a directory.
 * PERFECTLY WORKING
 */
void sfs_ls();
/**
 * Opens a file with the specified filename.
 *
 * PERFECTLY WORKING
 */
int sfs_open(char * name);

/**
 * Closes the specified file if it is found.
 *
 * PERFECTLY WORKING
 */
int sfs_close(int fileID);

/**
 * This function is not working properly.
 */
int sfs_write(int fileID, char * buf, int length);

/**
 * Most of the work is done in the util header file.
 * PERFECTLY WORKING WITH GOOD WRITE FUNCTION
 */
int sfs_read(int fileID, char * buf, int length);




#endif