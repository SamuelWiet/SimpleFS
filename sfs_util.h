#ifndef SFS_API_UTIL
#define SFS_API_UTIL

#include "sfs_header.h"
#include "disk_emu.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

/* Shared virtual disk buffer (defined in sfs_util.c) */
extern char vd[100][30000];
extern int fatID;

/* FAT and directory utility API */
void FAT_init(FileAllocationTable * fat);
int FAT_getFreeNode(FileAllocationTable * fat);
char * FAT_getPartFile(FileDescriptor file, FileAllocationTable fat, int length);
char * FAT_getFullFile(FileDescriptor file, FileAllocationTable fat);

void DirectoryDescriptor_init(DirectoryDescriptor * root);
int DirectoryDescriptor_getFreeSpot(DirectoryDescriptor * root);
int getIndexOfFileInDirectory(char* name, DirectoryDescriptor * rootDir);

void FileDescriptor_createFile(char* name, FileDescriptor * file);
int FileDescriptor_getNextWritable(FileDescriptor * file, FileAllocationTable * fat);

void VirtualDisk_write(int address, int length, void * buff);
void VirtualDisk_read(int address, void * buff, int length);

int FreeBlockList_getBlock(freeblocklist * fbl);

#endif

