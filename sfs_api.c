//
// Created by samuel on 1/12/26.
//

#include "disk_emu.h"
#include "sfs_header.h"

DirectoryDescriptor root;
FileAllocationTable fat;

#include "sfs_util.h"
#include "sfs_api.h"

/**
 * Creates or open a disk to read and write from.
 */
void mksfs(int fresh) {

	if (fresh) {
		init_fresh_disk("disk.sfs", sizeof(DiskBlock), NB_BLOCK);

		FAT_init(&fat);
		DirectoryDescriptor_init(&root);

		/* write DirectoryDescriptor and FAT wrapped in DiskBlock so write_blocks
		   copies a full DiskBlock from a buffer that is at least BLOCK_SIZE bytes */
		{
			DiskBlock *db = malloc(sizeof(DiskBlock));
			if (!db) {
				fprintf(stderr, "Error: malloc failed in mksfs\n");
				exit(1);
			}
			db->dd = root;
			write_blocks(0, 1, (void *)db);
			free(db);
		}
		{
			DiskBlock *db = malloc(sizeof(DiskBlock));
			if (!db) {
				fprintf(stderr, "Error: malloc failed in mksfs\n");
				exit(1);
			}
			db->fat = fat;
			write_blocks(1, 1, (void *)db);
			free(db);
		}

	} else {

		init_disk("disk.sfs", sizeof(DiskBlock), NB_BLOCK);
		{
			DiskBlock *db = malloc(sizeof(DiskBlock));
			if (!db) {
				fprintf(stderr, "Error: malloc failed in mksfs (load)\n");
				exit(1);
			}
			read_blocks(1, 1, (void *)db);
			fat = db->fat;
			free(db);
		}
		{
			DiskBlock *db = malloc(sizeof(DiskBlock));
			if (!db) {
				fprintf(stderr, "Error: malloc failed in mksfs (load)\n");
				exit(1);
			}
			read_blocks(0, 1, (void *)db);
			root = db->dd;
			free(db);
		}
	}
}

void persist_headers(void) {
    DiskBlock *db = malloc(sizeof(DiskBlock));
    if (!db) {
        fprintf(stderr, "Error: malloc failed in persist_headers\n");
        exit(1);
    }
    db->dd = root;
    write_blocks(0, 1, (void *)db);
    db->fat = fat;
    write_blocks(1, 1, (void *)db);
    free(db);
}

/**
 * Displays the content of a directory.
 * PERFECTLY WORKING
 */
void sfs_ls() {
	int i;
	printf("\n");
	for (i = 0; i < MAX_FILE; i++)
	{
		if(root.table[i].size > 0)
		{
			int kb = root.table[i].size / 1024;
			char * tm = ctime(&root.table[i].timestamp);
			tm[24] = '\0';
			printf("%25s\t%dKB\t%s\n", tm, kb, root.table[i].filename);
		}
	}
}

/**
 * Opens a file with the specified filename.
 *
 * PERFECTLY WORKING
 */
int sfs_open(char * name) {

    int fileID = getIndexOfFileInDirectory(name, &root);
    if (fileID != -1) {
        /* If fileID >= 0, return it after marking opened; otherwise (e.g. -2)
           propagate the error code (-2 means file is already open per util). */
        if (fileID >= 0) {
            root.table[fileID].fas.opened = 1;
            root.table[fileID].fas.rd_ptr = 0;
            /* persist headers */
            persist_headers();
        }
        return fileID;
    }

    /* Create a new file in the directory */
    fileID = DirectoryDescriptor_getFreeSpot(&root);
    FileDescriptor_createFile(name, &(root.table[fileID]));

    /* Assign a free block to the file and mark it as end of file */
    int alloc = FAT_getFreeNode(&fat);
    if (alloc < 0) {
        fprintf(stderr, "Error: no free FAT node when creating file %s\n", name);
        return -1;
    }
    root.table[fileID].fat_index = alloc;
    fat.table[alloc].next = EOF;

    persist_headers();

    return fileID;
}

/**
 * Closes the specified file if it is found.
 *
 * PERFECTLY WORKING
 */
int sfs_close(int fileID) {

    if (fileID < 0 || fileID >= MAX_FILE || root.table[fileID].size == EMPTY) {
        fprintf(stderr, "Error: Close: File #%d does not exist.", fileID);
        return -1;
    }
    root.table[fileID].fas.opened = 0;
    return 0;
}

/**
 * This function is not working properly.
 */
int sfs_write(int fileID, char * buf, int length) {

    if (fileID < 0 || fileID >= MAX_FILE || root.table[fileID].size == EMPTY) {
        fprintf(stderr, "Error: Write: File #%d does not exist.\n", fileID);
        return -1;
    }
    if (!root.table[fileID].fas.opened) {
        fprintf(stderr, "Error: Write: File #%d is not open.\n", fileID);
        return -1;
    }

    int blockSize = sizeof(DiskBlock);
    int remaining = length;
    int written = 0;

    /* find the node that contains the current EOF */
    int node = root.table[fileID].fat_index;
    int nodeStart = 0;
    int fileSize = root.table[fileID].size;
    while (node != EOF && nodeStart + blockSize <= fileSize) {
        nodeStart += blockSize;
        node = fat.table[node].next;
    }
    if (node == EOF) {
        fprintf(stderr, "Error: Write: corrupted FAT chain for file %d\n", fileID);
        return -1;
    }

    /* offset inside current block where new data should be appended */
    int startInBlock = fileSize - nodeStart;
    if (startInBlock < 0) startInBlock = 0;

    while (remaining > 0) {
        /* read the current block to preserve existing bytes */
        char *blockbuf = malloc(blockSize);
        if (!blockbuf) {
            fprintf(stderr, "Error: malloc failed in sfs_write\n");
            return written;
        }
        memset(blockbuf, 0, blockSize);
        /* read existing block content (if any) */
        if (read_blocks(fat.table[node].db_index, 1, blockbuf) != 1) {
            /* treat as empty block */
            memset(blockbuf, 0, blockSize);
        }

        int avail = blockSize - startInBlock;
        int tocopy = remaining < avail ? remaining : avail;
        memcpy(blockbuf + startInBlock, buf + written, tocopy);

        /* write back the modified block */
        if (write_blocks(fat.table[node].db_index, 1, blockbuf) != 1) {
            fprintf(stderr, "Error: Disk write failed.\n");
            free(blockbuf);
            break;
        }

        written += tocopy;
        remaining -= tocopy;
        root.table[fileID].size += tocopy;

        free(blockbuf);

        /* advance to next block */
        if (remaining > 0) {
            if (fat.table[node].next == EOF) {
                int newNode = FAT_getFreeNode(&fat);
                fat.table[node].next = newNode;
                fat.table[newNode].next = EOF;
                node = newNode;
            } else {
                node = fat.table[node].next;
            }
            startInBlock = 0;
        }
    }

    /* persist headers */
    persist_headers();

    return written;
}

int sfs_read(int fileID, char * buf, int length) {

    if (fileID < 0 || fileID >= MAX_FILE || root.table[fileID].size == EMPTY) {
        fprintf(stderr, "Error: Read: File #%d does not exist.\n", fileID);
        return -1;
    }
    if (!root.table[fileID].fas.opened) {
        fprintf(stderr, "Error: Read: File #%d is not open.\n", fileID);
        return -1;
    }

    int fileSize = root.table[fileID].size;
    int rd_ptr = root.table[fileID].fas.rd_ptr; /* byte offset */
    if (rd_ptr >= fileSize) return 0; /* nothing to read */

    int toRead = (length < (fileSize - rd_ptr)) ? length : (fileSize - rd_ptr);
    int blockSize = sizeof(DiskBlock);

    /* find starting node */
    int node = root.table[fileID].fat_index;
    int nodeStart = 0;
    while (node != EOF && nodeStart + blockSize <= rd_ptr) {
        nodeStart += blockSize;
        node = fat.table[node].next;
    }

    int copied = 0;
    while (copied < toRead && node != EOF) {
        char *blockbuf = malloc(blockSize);
        if (!blockbuf) break;
        if (read_blocks(fat.table[node].db_index, 1, blockbuf) != 1) {
            free(blockbuf);
            break;
        }

        int startInBlock = rd_ptr - nodeStart;
        if (startInBlock < 0) startInBlock = 0;
        int avail = blockSize - startInBlock;
        int need = toRead - copied;
        int take = avail < need ? avail : need;
        memcpy(buf + copied, blockbuf + startInBlock, take);
        copied += take;
        rd_ptr += take;
        nodeStart += blockSize;
        node = fat.table[node].next;
        free(blockbuf);
    }

    root.table[fileID].fas.rd_ptr = rd_ptr;
    return copied;
}


