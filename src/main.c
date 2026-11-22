#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "log_reader.h"

int main(int argc, char *argv[])
{
    if (argc < 2 || strcmp(argv[1], "-h") == 0) {
        printf("Usage:(sudo) %s <device-or-image> [-print_data] [-csv <output.csv>] [-bin <output.bin>]\n", argv[0]);
        return argc < 2 ? 0 : 0;
    }

    const char *path = argv[1];
    bool bin_enable = false, csv_enable = false, print_enable = false;
    const char *bin_filename = NULL, *csv_filename = NULL;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "-print_data") == 0) {
            print_enable = true;
        } else if (strcmp(argv[i], "-csv") == 0 && i + 1 < argc) {
            csv_enable = true;
            csv_filename = argv[++i];
        } else if (strcmp(argv[i], "-bin") == 0 && i + 1 < argc) {
            bin_enable = true;
            bin_filename = argv[++i];
        } else {
            fprintf(stderr, "Unknown or incomplete argument: %s\n", argv[i]);
            return 2;
        }
    }

    return INITIALIZE_SD_CARD(path, bin_enable, bin_filename, csv_enable, csv_filename, print_enable);
}
