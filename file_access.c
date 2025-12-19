#include <stdio.h>
#include "cmdline.h"
#include "file_access.h"
#include "cmds.h"
#include <string.h>
#include <stdlib.h>

fileinfo_t file_track[4];

rc_t
file_read(uint64_t space, uint64_t addr, uint width, void *bufp)
{
    uint slot = (space >> 8) & 0xff;
    if (file_track[slot].fp == NULL) {
        file_track[slot].fp = fopen(file_track[slot].filename, "r");
        if (file_track[slot].fp == NULL) {
            printf("Failed to open \"%s\" for read\n",
                   file_track[slot].filename);
            return (RC_FAILURE);
        }
    }
    if ((fseek(file_track[slot].fp, addr, SEEK_SET) == -1) ||
        (fread(bufp, width, 1, file_track[slot].fp) != 1)) {
        printf("Failed to read ");
        print_addr(space, addr);
        printf("\n");
        return (RC_FAILURE);
    }
    return (RC_SUCCESS);
}

rc_t
file_write(uint64_t space, uint64_t addr, uint width, void *bufp)
{
    uint slot = (space >> 8) & 0xff;
    if (file_track[slot].fp == NULL) {
        file_track[slot].fp = fopen(file_track[slot].filename, "r+");
        if (file_track[slot].fp == NULL)
            file_track[slot].fp = fopen(file_track[slot].filename, "w");
        if (file_track[slot].fp == NULL) {
            printf("Failed to open \"%s\" for write\n",
                   file_track[slot].filename);
            return (RC_FAILURE);
        }
    }
    if ((fseek(file_track[slot].fp, addr, SEEK_SET) == -1) ||
        (fwrite(bufp, width, 1, file_track[slot].fp) != 1)) {
        printf("Failed to write ");
        print_addr(space, addr);
        printf("\n");
        return (RC_FAILURE);
    }
    return (RC_SUCCESS);
}

void
file_cleanup_handles(void)
{
    uint slot;
    for (slot = 0; slot < ARRAY_SIZE(file_track); slot++) {
        if (file_track[slot].fp != NULL) {
            fclose(file_track[slot].fp);
            file_track[slot].fp = NULL;
        }
        if (file_track[slot].filename != NULL) {
            free(file_track[slot].filename);
            file_track[slot].filename = NULL;
        }
    }
}

/* This function returns the allocated slot number */
uint
file_track_filename(const char *name, size_t len)
{
    uint  slot;
    char *filename;

    for (slot = 0; slot < ARRAY_SIZE(file_track); slot++)
        if (file_track[slot].filename == NULL)
            break;

    if (slot >= ARRAY_SIZE(file_track)) {
        printf("Internal error: no file slots available\n");
        return (-1);
    }

    filename = malloc(len + 1);
    strncpy(filename, name, len);
    filename[len] = '\0';
    file_track[slot].filename = filename;
    file_track[slot].fp       = NULL;
    file_track[slot].mode     = FMODE_UNKNOWN;
    return (slot);
}
