#include "sfs_util.h"

char vd[100][30000];
int fatID = 0;

void FAT_init(FileAllocationTable * fat) {

	for (fatID = 2; fatID < FAT_SIZE - 1; fatID++) {
		/* Store the block index, not a byte offset. read_blocks expects
		   block indices. Mark nodes free initially. */
		fat->table[fatID].db_index 	= fatID;
		fat->table[fatID].next		= EOF;
		fat->table[fatID].free 		= 1;
	}
	/* start count at 1 so the allocation logic below returns >=2 on first
	   calls (consistent with initialization above starting at 2). */
	fat->count = 1;
}

int FAT_getFreeNode(FileAllocationTable * fat) {

	if (++fat->count < FAT_SIZE - 1) {
		/* mark node as used before returning */
		fat->table[fat->count].free = 0;
		return fat->count;
	} else {

		int i;
		for (i = 2; i < FAT_SIZE - 1; i++) {
			if (fat->table[i].free) {
				fat->table[i].free = 0;
				return i;
			}
		}
	}
	fat->count--;
	fprintf(stderr, "Error: Cannot get free block in FAT.\n");
	exit(0);
	return -5;
}

char * FAT_getPartFile(FileDescriptor file, FileAllocationTable fat, int length) {

	int curr, red = 0;	// 'red' counts blocks read
	int positionOnDisk = 0;
	void * positionOnBuff = 0;
	/* temporary buffer to collect data by block */
	char *buff = malloc(length);
	if (!buff) return NULL;
	curr = file.fat_index;

	for (; red * sizeof(DiskBlock) < length && curr != EOF; curr = fat.table[curr].next, red++) {

		positionOnDisk = fat.table[curr].db_index; /* this is a block index */
		positionOnBuff = (void *) ((char *)buff + red * sizeof(DiskBlock));
		/* read exactly one block (DiskBlock) into the target position */
		if (read_blocks(positionOnDisk, 1, positionOnBuff) != 1) {
			/* read failed or out of bounds */
			break;
		}
	}

	/* Return a heap buffer of the requested length. Note: the caller must
	   copy only the bytes they need and free the returned buffer. */
	char *ret = malloc(length);
	if (ret) memcpy(ret, buff, length);
	free(buff);
	return ret;
}

char * FAT_getFullFile(FileDescriptor file, FileAllocationTable fat) {

	return FAT_getPartFile(file, fat, file.size);
}

void DirectoryDescriptor_init(DirectoryDescriptor * root) {

	int i;
	for (i = 0; i < MAX_FILE; i++) {
		root->table[i].size = EMPTY;
		root->table[i].filename[0] = '\0';
		root->table[i].fas.opened = 0;
		root->table[i].fas.rd_ptr = 0;
		root->table[i].fas.wr_ptr = 0;
	}
	/* Ensure the file counter starts at -1 so DirectoryDescriptor_getFreeSpot
	   increments properly and returns 0 on first call. */
	root->count = -1;
}

int DirectoryDescriptor_getFreeSpot(DirectoryDescriptor * root) {

	if (++root->count < MAX_FILE) {
		return root->count;
	} else {

		int i;
		for (i = 0; i < MAX_FILE; i++) {
			if (root->table[i].size == EMPTY) {
				return i;
			}
		}
	}
	root->count--;
	fprintf(stderr, "Error: Cannot get free block in DD.\n");
	exit(0);
	return -1;
}

int getIndexOfFileInDirectory(char* name, DirectoryDescriptor * rootDir) {

	int i, fileID = -1;

	// Looks for the file and gets its index.
	for(i = 0; i < MAX_FILE; i++) {
		if (!strcmp(rootDir->table[i].filename, name)) {
			fileID = i;
			break;
		}
	}

	// Checks if the file was correctly closed.
	if ( fileID != -1) {
		if(rootDir->table[fileID].fas.opened == 1) {
			//fprintf(stderr, "Warning: '%s' is already open.\n", name);
			return -2;
		}
	}

	return fileID;
}

void FileDescriptor_createFile(char* name, FileDescriptor * file) {

	strcpy(file->filename, name);
	file->fas.opened = 1;
	file->fas.rd_ptr = 0;
	file->fas.wr_ptr = 0;
	file->size       = 0;
	file->timestamp  = time(NULL);
}

int FileDescriptor_getNextWritable(FileDescriptor * file, FileAllocationTable * fat) {

	int currNode = file->fat_index;

	// Goes to the end of the file.
	while (fat->table[currNode].next != EOF) {
		currNode = fat->table[currNode].next;
		file->fas.wr_ptr = currNode;
	}

	int nextNode = FAT_getFreeNode(fat);
	fat->table[currNode].next = nextNode;
	fat->table[nextNode].next = EOF;

	/* Return the block index of the newly allocated node, or -1 on error */
	return nextNode < 0 ? -1 : fat->table[nextNode].db_index;
}

void VirtualDisk_write(int address, int length, void * buff) {

	char * string = (char *) buff;
	strncpy(vd[address], string, length);
	//vd[address][pos+1] = '\0';
}

void VirtualDisk_read(int address, void * buff, int length) {

	char * string = (char *) buff;
	vd[address][length] = '\0';
	strncpy(string, vd[address], length);
}

int FreeBlockList_getBlock(freeblocklist * fbl) {

	int i;
	for(i = 2; i < NB_BLOCK - 1; i++) {
		if (fbl->list[i] == 0) {
			fbl->list[i] = 1;
			return i;
		}
	}
	return -1;
}
