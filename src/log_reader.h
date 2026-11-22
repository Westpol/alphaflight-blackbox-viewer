#ifndef LOG_READER_H
#define LOG_READER_H

#include <stdint.h>
#include <stdbool.h>

#define BLOCK_SIZE 4096u
#define SUPERBLOCK_ADDRESS_START 98u
#define METADATA_VERSION 0u
#define SUPERBLOCK_MAGIC 0x4B4C4253u
#define METADATA_MAGIC  0x444D4C46u

int INITIALIZE_SD_CARD(const char *path,
                       bool enable_bin_file, const char *bin_filename,
                       bool enable_csv_file, const char *csv_filename,
                       bool print_enable);

#endif
