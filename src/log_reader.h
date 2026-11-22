#ifndef LOG_READER_H
#define LOG_READER_H

#include "stdint.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdbool.h"

#define BLOCK_SIZE 512
#define SUPERBLOCK_INDEX 99
#define LOG_METADATA_BLOCK_START 100
#define LOG_FILES_PER_METADATA_BLOCK 10
#define ENABLE_CRC false
#define VERBOSE_OUTPUT false

#define SUPERBLOCK_MAGIC 0xFA55C0DE
#define LOG_METADATA_BLOCK_MAGIC 0xC5D4250A
#define LOG_METADATA_MAGIC 0xA1F17E5C

#define LOG_FRAME_START_MAGIC 0xC8
#define LOG_FRAME_END_MAGIC 0x9A

typedef struct __attribute__((packed)){
    uint32_t magic;     // 0x464C4D44 / "FLMD"
    uint32_t version;

    uint32_t flight_num_abs;
    uint32_t timestamp;
    uint32_t start_ms;
    uint32_t end_ms;
    int32_t lat;
    int32_t lon;

    uint32_t start_block;
    uint32_t end_block;
    uint32_t flags; // first bit if log started, second bit if log ended successfully
} metadata;

typedef struct __attribute__((packed)){
    uint32_t magic;     // 0x53424C4B / "SBLK"
    uint32_t version;
    uint32_t seq;
    uint32_t curr_flight_num_rel;
    uint32_t curr_flight_num_abs;
    uint32_t flags;     // corrupted, full, etc.
    uint32_t metadata_block_start;
    uint32_t metadata_block_end;
    uint32_t data_block_start;
    uint32_t data_block_end;
} superblock;


int INITIALIZE_SD_CARD(const char* PATH, bool ENABLE_BIN_FILE, const char* BIN_FILENAME, bool ENABLE_CSV_FILE, const char* CSV_FILENAME, bool PRINT_ENABLE);

#endif