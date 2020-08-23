#ifndef _SFILE_H
#define _SFILE_H

rc_t file_read(uint64_t space, uint64_t addr, uint width, void *bufp);
rc_t file_write(uint64_t space, uint64_t addr, uint width, void *bufp);

#define FMODE_UNKNOWN 0
#define FMODE_READ    1
#define FMODE_WRITE   2

typedef struct {
    FILE *fp;
    char *filename;
    uint  mode;
} fileinfo_t;
extern fileinfo_t file_track[4];

void file_cleanup_handles(void);
uint file_track_filename(const char *name, size_t len);

#endif

