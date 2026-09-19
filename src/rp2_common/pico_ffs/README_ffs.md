# "femto filing system" - A tiny footprint, lightweight flash filing system for RP2xx0
## Intended audience
It is recommended that the reader be at least somewhat familiar with the usage of the Raspberry Pi Pico SDK (henceforth the SDK) and operation of the RP2040 & RP2350 microcontroller devices.

## Introduction
This document describes the operation and intended applications of the "femto filing system", hereinafter referred to as **`ffs`** and styled in lower-case.

ffs is a tiny footprint, lightweight flash filing system which is recent addition to the SDK.  ffs is intended to hold configuration and other small data sets which require non-volatile storage; good examples are WiFi SSID and password settings, server and client details, user selected options, etc. etc.

Note: ffs does *not* in itself provide any security - data is stored in regular flash memory in clear text form.  But if needed, ffs can form part of a secure storage strategy if data stored in ffs is *first* encrypted from within the secure environment.  On retrieval, the data must be decrypted, again from within the secure environment of course.

The RP2350 has been designed for secure operation, please see the [RP2350 datasheet]( https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf ) for further details.

Notably: ffs is designed to provide storage integrity in that if a file write operation is interrupted by power-off or other reset event, ffs:
- Will (on reboot) repair the filing system so as to put it again in a usable state.
- Will (by design) leave the immediately previous version of the same file intact and available for retrieval.

## ffs Design
Note: For a description of the API functions, please see the `ffs.h` header file - this information is not replicated here.

Note2: Although the Pico SDK flash API is actually provided by the device bootrom, it is still considered part of the "Pico SDK" and is referred to here as such.

### Flash offsets vs memory addresses
A perhaps non-obvious detail is that any code that uses the Pico SDK to:

- Erase and or write to flash memory.
- Read flash partition information (only RP2350+).

Must and will, use and retrieve, **flash offset** values to identify flash locations.  Legal flash offset values start at 0, for the first byte location in flash, and so range between `0` and `PICO_FLASH_SIZE_BYTES - 1`.

The ffs module largely makes use of offset values and most variables are named as such and use the `uint32_t` type.

But, directly reading from flash, by execution and/or de-referencing a pointer, requires a flash **memory address** (rather than an offset).  On RP2xxx the flash memory addresses start at `0x10000000` and the memory itself may "reflect" multiple times after this address depending on flash size, device and device set-up.

An additional complication is that on RP2350+, it is possible to *re*map a different area of flash memory to (appear) to start at `0x10000000`.  This is to be able execute separate, but identically built and located images, directly from flash.

To allow ffs to operate on RP2350+ within a flash remapped runtime environment, the source code makes use of the **un**remapped flash memory access provided by the device hardware and located at `0x14000000`.

The ffs source code defines the `FL_OFFSET_TO_READ_ADDR(_o)` macro which is used to generate memory pointer values from a flash offsets and obfuscate the above detail from (most) of the source.  The ffs design does not ever need to convert from a memory address to a flash offset.

### Organisation of ffs flash storage
For its operation, ffs requires a dedicated area of flash memory to be organised into **two** discrete "chunks".  At any point, *one* chunk will be used to store new records in flash, the other chunk is left pre-prepared but fallow.

ffs stores discrete data items in a *list* of "records" (aka files) the active chunk.  The design ensures that, as each new file is written, the virgin area of flash immediately after the last programmed file is used.   Each file record uses a "file header" (see below).  The file header structure contains a status field which is used by ffs to support its integrity protection mechanism - more on this below.

When, due to increasing flash storage use, new write data cannot be entirely written within the current chunk, it will be written at the start of the "other" chunk.  At that point, all current valid records are then copied from the (now) "old" chunk to the newly active chunk.  On successful completion of this copy operation, which is non-destructively interruptible, the old chunk flash area is erased.  A special file at the start of each chunk is used as a marker to indicate the status of the chunk: active or fallow.  The chunk markers uses the record's status field and operates in a similar manner to a user accessible file.

This "two chunk" design results in a first-order limitation: All the stored valid files must fit into a *single* chunk.  This obviously holds true for both a single large file or a collection of mixed size files.

Because flash erasure is done on a per-sector basis, an ffs chunk **must** be an exact multiple of number of flash sectors in size *and* chunks must start on flash sector boundaries.

The total flash area used by ffs is therefore `2 * N * FLASH_SECTOR_SIZE` where `N` is the number of sectors allocated for a single chunk.  Please also see [Dimensions](###Dimensions)

Note: The order files are stored in flash does not matter since because when a file is to be read all records in the active chuck  will be scanned for the legitimate (last) version of the required file.  The flash scanning and file validating algorithm is quick to operate.

### File IDs, hierarchy etc.
There is no hierarchy of directories in ffs, it is a "flat" list of files, no file permission mechanism is provided.
ffs does not use human readable filenames to distinguish files but instead uses a "file identifier" or `file_id` which are stored in a single basic C data type.  File IDs should be defined on per-application basis using `#define` or an enumerated type.

If human readable filenames are required, the translation between file_id and ASCII text can be done by look-up, either by a hard coded table or possibly by storage within a file in ffs itself.

The length of the data is stored within the file header, which prefaces the data in the file record in flash memory.  Stored data is treated as raw or binary data, there are no reserved values within the data.  It is possible to store and retrieve *zero* data length files.

### File header structure
At the time of writing, the `ffs_file_hdr_t` file header structure is composed of:

- Two 32-bit words containing "magic values" to allow identification of this as a file header structure. Two words were chosen over one due to paranoia!
One magic word is ASCII readable, to aid debug using memory contents inspection, the other a random collection of bytes.
- An 8-bit "file_id" - the unique reference used by the API to *select* a file for reading, writing or deletion.
- A 16-bit data length - number of octets of file data starting immediately after the file header (can be 0).
- An 8-bit file "status" - used to indicate the provenance of the file record, see below.
- Two 32-bit CRC storage fields, `crc0` and `crc1`, see below.

### Design notes
In no particular order:

- ffs is *not* intended to provide for all possible applications, just to provide the smallest viable storage facility for a moderate number.
- One `file_id` value is reserved for ffs internal use and is used to signify a "chunk header".  A chunk header is a zero data length file which is only ever located at exactly at the start of a chunk.
- The ffs API will check that file IDs range between `0` and `FFS_MAX_VALID_FILE_ID` (inclusive) and will report `PICO_ERROR_INVALID_ARG` if FFS_MAX_VALID_FILE_ID is exceeded. With an 8-bit `file_id` FFS_MAX_VALID_FILE_ID is defined as 254 providing for 255 unique file IDs (with value 255 being used for the chunk header).
- If the size of the fields defined in `ffs_file_hdr_t` prove too restrictive for some specific applications, end users may of course modify the source code themselves. The API uses the SDK's *32-bit* `uint` type for the file ID and data length arguments so that application code will not need changing if the ffs source is modified to extend the range of allowable arguments over those described above.
- No version field is included because a "runtime updatable" file system is not envisaged or trivial!
- If ffs proves totally unsuitable for a given application, nothing in the Pico SDK precludes the user integrating an entirely different flash filing system.

### Storage integrity
As noted, ffs is designed to provide storage integrity; this is accomplished by using (a) a three-value "status" field and (b) the two CRC32 check values for each file (crc0 and crc1).

A new file is written in three discrete steps:

1. A file header structure is generated in memory with the status field set to all `1`s, this keeps the field in the flash "unprogrammed" and indicates a `nascent` file.  The header's first CRC value, crc0, is calculated from the header including its the status and the data to be programmed (if any).  The header is written to flash immediately followed by the data.  At this point the header's crc1 field has also been left as all `1`s

2. On successful programming, the flash is scanned for a `valid` file with the same file_id. i.e. the current copy.  *If* such a file is located, its status field is marked as `deprecated` since there is now a stored nascent file which takes priority.  The deprecated status is written using all `0`s ensuring that file can't ever be returned to valid or nascent - that area of flash must be erased first.

3. The new file's status field is set to valid.  The valid state is encoded as a mixture of `1`s and `0`s meaning it is unequivocably neither nascent or deprecated and can never become nascent without erasure.  At this point the crc1 value is calculated, using crc0 and the newly set valid status field.  The updated header with the new status and updated crc1 is written to flash, "validating" the file storage location.

Note: Steps (2) and (3) above make use of the "Write more zeros" flash programming methodology see [Flash write page cache](###Flashwritepagecache) for implementation.

This system design ensures there is only **one** identifiable copy of each unique file (based on file_id) in ffs flash at any one time.  During ffs initialisation, the ffs flash area is scanned for nascent files and, if found, steps (2) and (3) are applied to ensure that there is subsequently only one copy of the file marked as valid. Deprecated files are ignored during file searches and will be erased at the next chunk swap.

### Atomicity
There is no concept of "opening a file" for subsequent read or write. Files in ffs are read, written, erased and listed in atomic and synchronous operations.
All data must be supplied to write a file, files can be read (direct from flash) using the supplied pointer up to the reported length of the file.  Due to the amount of context data held within the module, ffs is explicitly *not* threadsafe.

### Dimensions
The ffs code is designed around some basic assumptions controlled by:
- Flash programming *page* size.
- Flash *sector* size.

Common QSPI flash devices, including all those used on RPi Pico products, use a 256 byte programming page and 4096 byte sectors.

The ffs source code picks up these dimensions from the `FLASH_PAGE_SIZE` and `FLASH_SECTOR_SIZE` definitions which are defined on a per-board basis in the SDK environment.

As noted, ffs is designed so that all data must fit in one flash chunk.  A design feature is that all file headers start on 32-bit word boundaries although the payload data can be any number of bytes (subject to the maximum length of `FFS_API_MAX_WRITE_LENGTH`).  By locating file headers on 32-bit boundaries the file search time is reduced compared to having to search on octet boundaries if "packed" file records were used.

At the time of writing, the file header structure is *five* 32-bit words in size. For the expected use cases, the header structure:
- Contains enough data for a sensible level of integrity checking.
- Is small enough in practice; it is not expected to need to store many dozens of different files.

### Flash write page cache
During operation, ffs makes use of the "Write more zeroes" approach which is applicable to flash memory which has an unprogrammed state of `1` and can program additional `0` bits in subsequent programming operations.  To permit the use of WMZ and indeed to provide a flexible/fast ffs internal flash programming API a "write page" cache is used.  All ffs writes to flash go through its write page caching mechanism.

To support the requirements of providing data integrity, the write page cache must be flushed into flash by programming before other fields in it are subsequently modified and the page programmed "again".
A "currently cached" page variable is maintained and is used to indicate when the cache is in use and mush be flushed to finalise an operation and/or flushed because new data to be written is beyond the limits of the current page.

### Dealing with virgin, used and corrupt flash
There is no need for any sort of explicit flash "formatting", ffs will work starting with totally unprogrammed flash.

Please note: Before commencing file operations, ffs *must* be initialised by calling `ffs_initialise()`.  This will check the flash state and automatically take steps to leave the ffs flash area ready for correct operation.  Note: This *may* include the explicit voiding of corrupt records and/or the completion of coalescing of valid files into a new chunk and erasure of the previous chunk.

### `ffsgen` Generating a initial filing system
Many systems will require some "pre-programmed" configuration data to be present in flash when the system is first booted.  The `ffsgen` tool will construct and write a binary object file from a *list* of file IDs and their respective source files contained, in a config file.

For details of `ffsgen` build and operation, please see: [README_ffsgen.md](../../../tools/ffsgen/README_ffsgen.md)

### Flash area definition
At *run time*, ffs will calculate the number of flash sectors to used for each *chunk*.  The total flash available for ffs use is the `end offset - start offset`.
In builds for RP2350 (and later) devices, the ffs start and end offsets are *read* from the ffs partition entry size.  In builds for RP2040, the ffs start and end offsets must be *defined*.  This is done in `ffs.h` using overridable defaults per Pico-SDK style.

Below are two examples of minimum sized ffs flash regions

#### RP2350
For ffs usage, the flash memory RP2350 must be formatted into partitions using the `picotool` utility; ffs will *expect* to be able to locate partition dedicated for its usage – please see the `FFS_DATA_PARTITION_ID` in `ffs.h`.
A suitable entry in a partition table entry would be:
```
    {
      "name": "ffs storage area",
      "id": "0x746d656673665f6f",
      "size": "8K",
      "families": ["data"],
      "permissions": {
        "secure": "rw",
        "nonsecure": "rw",
        "bootloader": "rw"
      }
    }
```
Self evidently, the other partition(s) allocated for code and/or other data storage must be dimensioned to allow for (at least) size of the above partition to be added to the partition table to allow them all to fit in flash memory.

### RP2040

RP2040 Linker segment

Because there is no flash partitioning mechanism on RP2040, the location and size of the ffs area in flash must be known at build time (or link time, see below) - it can *not* be discovered at run time.

At the time of writing, `ffs.h` uses defaults, which can be overridden, to define the start and end of the ffs flash regions.  These defaults will put the ffs region at the very top of flash memory, specifically:
```
#define FFS_RP2040_FLASH_END_OFFSET    PICO_FLASH_SIZE_BYTES
#define FFS_RP2040_FLASH_START_OFFSET  (PICO_FLASH_SIZE_BYTES - (2 * FLASH_SECTOR_SIZE))
```

FIXME: A better approach, for which support may be added if time allows, would be the explicit definition on an ffs segment in the linker script.  This would provide "protection" of the desired ffs region in that the link step will fail if segments overlap or fail to fit in flash memory.  Some small modifications in ffs source would allow the the RP2040 build to pick-up the start offset and size of the ffs region at link time.
