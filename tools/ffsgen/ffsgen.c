/**
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Utility for constructing a binary blob, for programming at the start of, an
 * otherwise virgin (erased), `ffs` memory area so that the ffs will have files
 * in it at start of day i.e. these need not be subsequently written at runtime.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "ffs_internal.h"

#define FFSGEN_VERSION              "v0.1"

// 4k is often one SPI flash sector size and is on Pico boards
#define FFSGEN_EXPECTED_SECTOR_SIZE     4096
#define NUM_OF_FIDS                     (FFS_MAX_VALID_FILE_ID + 1)
#define MAX_LINE_LEN                    100

// Collect module globals into a single struct
typedef struct {
    ffs_file_hdr_t hdr_buff;
    uint chunk_size;
    uint next_write_offset;
    unsigned char *chunk_mem;
} ffsgen_t;

static ffsgen_t ffsgen;


// NOTE: This function automatically advances the write address used as
// data is written so that callers can easily store consecutive items.
static int write_to_chunk_mem(uint *ptr_to_wr_offset, const void *data, uint length) {

    const char *rdptr = (char *) data;
    while (0 < length--) {

        // NOTE: Must leave 1 spare byte to avoid round-up errors in ffs itself
        if (*ptr_to_wr_offset >= ffsgen.chunk_size-1) {
            printf("error - ffs chunk memory filled!\n");
            return -1;
        }

        ffsgen.chunk_mem[(*ptr_to_wr_offset)++] = *rdptr++;
    }

    return 0;
}


static int write_chunk_header_into_chunkmem(void) {

    file_header_generate(&ffsgen.hdr_buff,  FILE_ID_CHUNK_HEADER, NULL, 0);
    file_header_validate(&ffsgen.hdr_buff);

    ffsgen.next_write_offset = 0; // Chunk header is always right at the start

    return write_to_chunk_mem(&ffsgen.next_write_offset,
                              &ffsgen.hdr_buff,
                              sizeof(ffsgen.hdr_buff));
}


static int write_new_file_into_chunkmem(uint file_id, const char *file_name) {

    int ret_error = 0;

    FILE *fp_ffs_file;
    if (NULL == (fp_ffs_file = fopen(file_name, "r"))) {
        printf("error - failed to open file '%s' for read, error: %s\n",
            file_name, strerror(errno));
        return -1;
    }

    // The file header structure contains words, it must start on a word boundary
    ffsgen.next_write_offset = ROUND_UP_TO_NEXT_32B_WORD(ffsgen.next_write_offset);

    uint header_offset = ffsgen.next_write_offset;
    uint save_hdr_ofst = header_offset;
    uint wrdata_offset = header_offset + sizeof(ffs_file_hdr_t);

    const char *wrdata = (const char *) &ffsgen.chunk_mem[wrdata_offset];
    uint data_len = 0;
    uint8_t fdata;

    // Read from file, write into chunk memory, byte by byte ...
    while (!ret_error && 1 == fread(&fdata, 1, 1, fp_ffs_file)) {
        ret_error = write_to_chunk_mem(&wrdata_offset, &fdata, 1);
        data_len++;
    }

    fclose(fp_ffs_file);

    // Set write offset ready for the *next* file and for reporting below
    ffsgen.next_write_offset = wrdata_offset;
    ffsgen.next_write_offset = ROUND_UP_TO_NEXT_32B_WORD(ffsgen.next_write_offset);

    if (ret_error) {
        printf("error - file '%s' will not fit!\n", file_name);
    }
    else {
        // Data written, now generate and write header - needs two steps
        file_header_generate(&ffsgen.hdr_buff, file_id, wrdata, data_len);
        ret_error = write_to_chunk_mem(&header_offset, &ffsgen.hdr_buff, sizeof(ffsgen.hdr_buff));
    }

    if (!ret_error) {
        file_header_validate(&ffsgen.hdr_buff);
        ret_error = write_to_chunk_mem(&save_hdr_ofst, &ffsgen.hdr_buff, sizeof(ffsgen.hdr_buff));
    }

    if (!ret_error) {
        printf("info - added file id: %i, '%s', data length: %i, space left in chunk: %i\n",
            file_id, file_name, data_len, ffsgen.chunk_size-ffsgen.next_write_offset-1);
    }

    return ret_error;
}


static int process_config_file(FILE *fp_config) {

    char line_buff[MAX_LINE_LEN];
    char *line;

    bool fid_in_use[NUM_OF_FIDS];
    for (int i = 0; i < NUM_OF_FIDS; i++) {
        fid_in_use[i] = false;
    }

    int ret_error = 0;

    while (!ret_error && NULL != (line = fgets(line_buff, MAX_LINE_LEN, fp_config))) {

        char file_name[MAX_LINE_LEN];
        uint file_id;
        int len = strlen(line);

        if (len && (line[len-1] == (char)'\r' || line[len-1] == (char)'\n')) {
            line[len-1] = 0;
        }

        if (0 == strlen(line) || line[0] == (char) '#') {
            continue; // skip
        }

        if (2 != sscanf(line, "%u %s", &file_id, file_name)) {
            printf("error - bad line arguments: '%s'\n", line);
            printf("        must be: 'file_id filename'\n");
            return -1;
        }

        if (FFS_MAX_VALID_FILE_ID < file_id) {
            printf("error - bad file id: %i\n", file_id);
            printf("        must be: 0 to %i (inc)\n", FFS_MAX_VALID_FILE_ID);
            return -1;
        }

        if (fid_in_use[file_id]) {
            printf("error - duplicate file id: %i\n", file_id);
            return -1;
        }

        fid_in_use[file_id] = true;

        ret_error = write_new_file_into_chunkmem(file_id, file_name);
    }

    return ret_error;
}


int main(int argc, char *argv[]) {

    ffsgen.chunk_size = FFSGEN_EXPECTED_SECTOR_SIZE;
    ffsgen.chunk_mem = NULL;

    FILE *fp_config = NULL;
    FILE *fp_output = NULL;

    int ret_error = 0;

    printf("ffs femto filing system blob builder tool, %s\n", FFSGEN_VERSION);

    if (argc != 3 && argc != 4) {
        printf("usage: %s <config_file> <output_file> [chunk_size]\n", argv[0]);
        printf("where <config_file> contains *1* line per input file of the form 'ID filename':\n");
        printf("# Lines starting with a '#' are ignored and can be used for comments\n");
        printf("0 file.txt\n");
        printf("1 file1.txt\n");
        printf("2 file2.bin\n");
        printf("etc. etc.\n");
        printf("File IDs must be (a) unique (b) in the range 0-254 (decimal)\n");
        printf("Filenames must locate a file or include a path and a filename\n");
        printf("The output file is binary. It must be programmed at the start of the (otherwise empty) ffs area\n");
        printf("The 'chunk_size' argument is optional, if not used the default is %u bytes\n", FFSGEN_EXPECTED_SECTOR_SIZE);
        return -1;
    }

    if (argc == 4) {
        if (1 != sscanf(argv[3], "%i", &ffsgen.chunk_size)) {
            printf("Bad chunk_size value, type '%s' for usage\n", argv[0]);
            return -1;
        }
        else if (ffsgen.chunk_size % FFSGEN_EXPECTED_SECTOR_SIZE) {
            printf("Warning - %i chunk size is not an integer multiple of the expected %i byte sector size\n",
                ffsgen.chunk_size, FFSGEN_EXPECTED_SECTOR_SIZE);
        }
    }
    else {
        printf("info - defaulting the chunk size to %i bytes\n", ffsgen.chunk_size);
    }

    if (NULL == (ffsgen.chunk_mem = malloc(ffsgen.chunk_size))) {
        printf("error - failed to allocate %i bytes of memory\n", ffsgen.chunk_size);
        return -1;
    }
    else {
        //  Make the blob all virgin flash values to start with.
        memset(ffsgen.chunk_mem, 0xFF, ffsgen.chunk_size);
    }

    if (NULL == (fp_config = fopen(argv[1], "r"))) {
        printf("error - failed to open config file '%s' for read, error: %s\n", argv[1], strerror(errno));
        ret_error = -1;
    }

    if (!ret_error) {
        ret_error = write_chunk_header_into_chunkmem();
    }

    if (!ret_error) {
        ret_error = process_config_file(fp_config);
    }

    if (!ret_error) {
        if (NULL == (fp_output = fopen(argv[2], "wb"))) {
            printf("error - failed to open output file '%s' for write, error: %s\n", argv[2], strerror(errno));
            ret_error = -1;
        }
    }

    if (!ret_error) {
        size_t wr_result = fwrite(ffsgen.chunk_mem, 1, ffsgen.chunk_size, fp_output);
        if (ffsgen.chunk_size == wr_result) {
            printf("info - success, wrote %i bytes to '%s'\n", ffsgen.chunk_size, argv[2]);
        }
        else {
            printf("error - failed to write data to '%s', returned: %li\n", argv[2], wr_result);
        }
    }

    if (NULL != ffsgen.chunk_mem) {
        free(ffsgen.chunk_mem);
    }
    if (NULL != fp_config) {
        fclose(fp_config);
    }
    if (NULL != fp_output) {
        fclose(fp_output);
    }

    return ret_error;
}
