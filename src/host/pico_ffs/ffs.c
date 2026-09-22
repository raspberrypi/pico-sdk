/*
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Host implementation of the femto flash filing system API: one file per id,
// stored as ffs_<id>.dat in the current directory.

#include "pico/ffs.h"

#include <stdio.h>
#include <dirent.h>

static char read_buffer[FFS_API_MAX_WRITE_LENGTH];

static void ffs_get_filename(uint file_id, char *buf, size_t buflen) {
    snprintf(buf, buflen, "ffs_%u.dat", file_id);
}

int ffs_initialise(void) {
    return PICO_OK;
}

int ffs_write(uint file_id, const char *wrdata, uint data_len) {
    FILE *fp;
    char filename[32];

    if (file_id > FFS_MAX_VALID_FILE_ID) {
        return PICO_ERROR_GENERIC;
    }

    if (data_len > FFS_API_MAX_WRITE_LENGTH) {
        return PICO_ERROR_GENERIC;
    }

    ffs_get_filename(file_id, filename, sizeof(filename));

    fp = fopen(filename, "wb");
    if (!fp) {
        return PICO_ERROR_GENERIC;
    }

    if (fwrite(wrdata, 1, data_len, fp) != data_len) {
        fclose(fp);
        return PICO_ERROR_GENERIC;
    }

    fclose(fp);
    return PICO_OK;
}

int ffs_read(uint file_id, const char **filedata) {
    FILE *fp;
    char filename[32];
    long file_size;
    size_t bytes_read;

    if (file_id > FFS_MAX_VALID_FILE_ID) {
        return PICO_ERROR_GENERIC;
    }

    ffs_get_filename(file_id, filename, sizeof(filename));

    fp = fopen(filename, "rb");
    if (!fp) {
        return PICO_ERROR_NOT_FOUND;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size < 0 || file_size > (long)FFS_API_MAX_WRITE_LENGTH) {
        fclose(fp);
        return PICO_ERROR_GENERIC;
    }

    bytes_read = fread(read_buffer, 1, file_size, fp);
    fclose(fp);

    if (bytes_read != (size_t)file_size) {
        return PICO_ERROR_GENERIC;
    }

    *filedata = read_buffer;
    return (int)file_size;
}

int ffs_delete(uint file_id) {
    char filename[32];

    if (file_id > FFS_MAX_VALID_FILE_ID) {
        return PICO_ERROR_GENERIC;
    }

    ffs_get_filename(file_id, filename, sizeof(filename));

    if (remove(filename) != 0) {
        return PICO_ERROR_NOT_FOUND;
    }

    return PICO_OK;
}

int ffs_list(file_info_t file_info[], uint max_infos) {
    DIR *dir;
    struct dirent *entry;
    uint count = 0;
    uint file_id;
    FILE *fp;
    long file_size;

    dir = opendir(".");
    if (!dir) {
        return PICO_ERROR_GENERIC;
    }

    while ((entry = readdir(dir)) != NULL && count < max_infos) {
        if (sscanf(entry->d_name, "ffs_%u.dat", &file_id) == 1) {
            if (file_id <= FFS_MAX_VALID_FILE_ID) {
                fp = fopen(entry->d_name, "rb");
                if (fp) {
                    fseek(fp, 0, SEEK_END);
                    file_size = ftell(fp);
                    fclose(fp);

                    if (file_size >= 0 && file_size <= (long)FFS_API_MAX_WRITE_LENGTH) {
                        file_info[count].file_id = file_id;
                        file_info[count].data_len = (uint)file_size;
                        count++;
                    }
                }
            }
        }
    }

    closedir(dir);
    return (int)count;
}
