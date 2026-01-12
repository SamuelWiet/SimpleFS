#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "sfs_util.h"
#include "sfs_header.h"

int main(void) {
    // FAT tests
    FileAllocationTable fat;
    FAT_init(&fat);
    // After init, nodes from 2 should have db_index == index
    assert(fat.table[2].db_index == 2);
    assert(fat.table[3].db_index == 3);
    // count should be 1 per implementation
    assert(fat.count >= 1);

    int a = FAT_getFreeNode(&fat);
    assert(a >= 2);
    // the node returned should be marked used
    assert(fat.table[a].free == 0);

    // Directory tests
    DirectoryDescriptor dd;
    DirectoryDescriptor_init(&dd);
    assert(dd.count == -1);
    int spot = DirectoryDescriptor_getFreeSpot(&dd);
    assert(spot == 0);
    assert(dd.table[0].size == EMPTY || dd.table[0].size == 0 || dd.table[0].size == EMPTY);

    // Create file descriptor
    FileDescriptor fd;
    FileDescriptor_createFile("TESTFILE", &fd);
    assert(fd.size == 0);
    assert(fd.fas.opened == 1);
    assert(strcmp(fd.filename, "TESTFILE") == 0);

    // FileDescriptor_getNextWritable should allocate a node for a file
    FileAllocationTable fat2;
    FAT_init(&fat2);
    // set file's fat_index to some allocated node
    fd.fat_index = FAT_getFreeNode(&fat2);
    int bix = FileDescriptor_getNextWritable(&fd, &fat2);
    // returns a block index (>=0) or -1 on error
    assert(bix >= 0);

    // VirtualDisk write/read basic sanity
    char bufw[16] = "hello, world\0";
    VirtualDisk_write(2, strlen(bufw)+1, bufw);
    char bufr[16];
    VirtualDisk_read(2, bufr, strlen(bufw)+1);
    // vd stores bytes but VirtualDisk_read writes a terminating '\0' at position length
    // compare prefix
    assert(strncmp(bufr, bufw, strlen(bufw)) == 0);

    printf("All unit tests passed\n");
    return 0;
}

