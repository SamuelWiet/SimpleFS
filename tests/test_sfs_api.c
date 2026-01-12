#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "sfs_api.h"

int main(void) {
    /* Initialize fresh file system */
    mksfs(1);

    /* Open/create a test file */
    char *name = "UNIT.TEST";
    int fd = sfs_open(name);
    if (fd < 0) {
        fprintf(stderr, "ERROR: sfs_open failed for %s\n", name);
        return 1;
    }

    /* Write a small payload */
    const char *payload = "HelloSFS";
    int len = (int)strlen(payload);
    int wrote = sfs_write(fd, (char *)payload, len);
    assert(wrote == len);

    /* Close and reopen */
    assert(sfs_close(fd) == 0);
    int fd2 = sfs_open(name);
    assert(fd2 >= 0);

    /* Read back */
    char buf[64];
    memset(buf, 0, sizeof(buf));
    int read = sfs_read(fd2, buf, len);
    assert(read == len);
    assert(memcmp(buf, payload, len) == 0);

    /* Clean up */
    assert(sfs_close(fd2) == 0);

    printf("sfs API basic test passed\n");
    return 0;
}

