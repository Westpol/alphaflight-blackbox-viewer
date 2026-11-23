#define _FILE_OFFSET_BITS 64
#include "log_reader.h"
#include "logging_structs.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t seq;
    uint32_t curr_flight_num_rel;
    uint32_t curr_flight_num_abs;
    uint32_t flags;
    uint32_t metadata_block_start;
    uint32_t metadata_block_end;
    uint32_t data_block_start;
    uint32_t data_block_end;
} superblock_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t flight_num_abs;
    uint32_t timestamp;
    uint32_t start_ms;
    uint32_t end_ms;
    int32_t lat;
    int32_t lon;
    uint32_t start_block;
    uint32_t end_block;
    uint32_t flags;
} metadata_t;

#define METADATA_PER_BLOCK (BLOCK_SIZE / sizeof(metadata_t))

static int g_fd = -1;
static superblock_t g_sb;
static metadata_t g_flight;

static int read_block(uint32_t block, uint8_t data[BLOCK_SIZE])
{
    off_t offset = (off_t)block * BLOCK_SIZE;
    if (lseek(g_fd, offset, SEEK_SET) != offset) {
        perror("lseek");
        return -1;
    }
    ssize_t n = read(g_fd, data, BLOCK_SIZE);
    if (n != (ssize_t)BLOCK_SIZE) {
        if (n < 0) perror("read");
        else fprintf(stderr, "Short read at block %u: %zd bytes\n", block, n);
        return -1;
    }
    return 0;
}

static int load_superblock(void)
{
    uint8_t raw[2][BLOCK_SIZE];
    superblock_t candidates[2] = {{0}};
    for (unsigned i = 0; i < 2; ++i) {
        if (read_block(SUPERBLOCK_ADDRESS_START + i, raw[i]) != 0)
            return -1;
        memcpy(&candidates[i], raw[i], sizeof(superblock_t));
    }

    int valid0 = candidates[0].magic == SUPERBLOCK_MAGIC;
    int valid1 = candidates[1].magic == SUPERBLOCK_MAGIC;
    if (!valid0 && !valid1) {
        fprintf(stderr, "No valid filesystem superblock found (expected magic %08" PRIX32 ").\n",
                SUPERBLOCK_MAGIC);
        return -1;
    }
    if (valid0 && (!valid1 || candidates[0].seq > candidates[1].seq))
        g_sb = candidates[0];
    else
        g_sb = candidates[1];

    if (g_sb.version != 0) {
        fprintf(stderr, "Warning: superblock version is %u, viewer expects version 0.\n",
                g_sb.version);
    }
    return 0;
}

static int load_flight_by_relative(uint32_t relative_num, metadata_t *result)
{
    uint32_t block = g_sb.metadata_block_start + relative_num / METADATA_PER_BLOCK;
    uint32_t index = relative_num % METADATA_PER_BLOCK;

    if (block > g_sb.metadata_block_end) {
        fprintf(stderr, "Metadata block %u is outside configured metadata range.\n", block);
        return -1;
    }

    uint8_t raw[BLOCK_SIZE];
    if (read_block(block, raw) != 0)
        return -1;

    memcpy(result, raw + index * sizeof(metadata_t), sizeof(metadata_t));
    if (result->magic != METADATA_MAGIC) {
        fprintf(stderr, "Invalid metadata magic for relative flight %u.\n", relative_num);
        return -1;
    }
    return 0;
}

static int find_flight(uint32_t absolute_num)
{
    /* The FC stores completed flights in relative-number order. */
    for (uint32_t rel = 0; rel < g_sb.curr_flight_num_rel; ++rel) {
        metadata_t m;
        if (load_flight_by_relative(rel, &m) == 0 && m.flight_num_abs == absolute_num) {
            g_flight = m;
            return 0;
        }
    }
    return -1;
}

static void print_record(const uint8_t *p, uint8_t id)
{
    switch (id) {
    case LOG_RECORD_TIMESTAMP: {
        log_timestamp_t r;
        memcpy(&r, p, sizeof r);
        printf("TIMESTAMP millis=%d\n", r.millis);
        break;
    }
    case LOG_RECORD_GPS: {
        log_gps_t r;
        memcpy(&r, p, sizeof r);
        printf("GPS time=%u lat=%d lon=%" PRId32
               " speed=%d height=%d course=%" PRId32
               " sats=%u\n", r.timestamp, r.lat, r.lon, r.speed, r.height,
               r.course, r.numSV);
        break;
    }
    case LOG_RECORD_IMU: {
        log_imu_t r;
        memcpy(&r, p, sizeof r);
        printf("IMU gyro=(%d,%d,%d) accel=(%d,%d,%d) quat=(%d,%d,%d,%d) pitch=%d roll=%d\n",
               r.wx, r.wy, r.wz, r.accx, r.accy, r.accz,
               r.quat_w, r.quat_x, r.quat_y, r.quat_z, r.pitch, r.roll);
        break;
    }
    case LOG_RECORD_SERVO: {
        log_servo_t r;
        memcpy(&r, p, sizeof r);
        printf("SERVO pwm=(%u,%u,%u,%u) dshot=%u\n",
               r.pwm[0], r.pwm[1], r.pwm[2], r.pwm[3], r.dshot);
        break;
    }
    default:
        printf("Unknown record id 0x%02X\n", id);
        break;
    }
}

static void csv_record(FILE *f, const uint8_t *p, uint8_t id)
{
    switch (id) {
    case LOG_RECORD_TIMESTAMP: {
        log_timestamp_t r; memcpy(&r, p, sizeof r);
        fprintf(f, "TIMESTAMP,%u\n", r.millis);
        break;
    }
    case LOG_RECORD_GPS: {
        log_gps_t r; memcpy(&r, p, sizeof r);
        fprintf(f, "GPS,%u,%u,%d,%d,%d,%d,%d\n",
                r.timestamp, r.numSV, r.lat, r.lon, r.speed, r.height, r.course);
        break;
    }
    case LOG_RECORD_IMU: {
        log_imu_t r; memcpy(&r, p, sizeof r);
        fprintf(f, "IMU,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                r.wx, r.wy, r.wz, r.accx, r.accy, r.accz,
                r.quat_w, r.quat_x, r.quat_y, r.quat_z, r.pitch, r.roll);
        break;
    }
    case LOG_RECORD_SERVO: {
        log_servo_t r; memcpy(&r, p, sizeof r);
        fprintf(f, "SERVO,%u,%u,%u,%u,%u\n",
                r.pwm[0], r.pwm[1], r.pwm[2], r.pwm[3], r.dshot);
        break;
    }
    default:
        fprintf(f, "UNKNOWN,%u\n", id);
        break;
    }
}

/* Each data block starts with the 4-byte absolute flight number.
   Records immediately follow; unlike the previous viewer format there are
   no frame-start/frame-end magic values or per-record length bytes. */
static int process_flight(FILE *csv, int print_enable)
{
    uint8_t block[BLOCK_SIZE];
    if (g_flight.start_block > g_flight.end_block ||
        g_flight.start_block < g_sb.data_block_start ||
        g_flight.end_block > g_sb.data_block_end) {
        fprintf(stderr, "Flight data block range is invalid.\n");
        return -1;
    }

    for (uint32_t b = g_flight.start_block; ; ++b) {
        if (read_block(b, block) != 0)
            return -1;

        uint32_t block_flight_num;
        memcpy(&block_flight_num, block, sizeof block_flight_num);
        fprintf(stderr, "Block %u: flight_num=%u\n", b, block_flight_num);
        if (block_flight_num != g_flight.flight_num_abs) {
            fprintf(stderr, "Warning: block %u belongs to flight %u, expected %u.\n", b, block_flight_num, g_flight.flight_num_abs);
        }

        size_t pos = sizeof(uint32_t);
        while (pos < BLOCK_SIZE) {
            uint8_t id = block[pos];
            if (id == 0)
                break; /* Remaining bytes are zero padding. */

            size_t record_size;
            switch (id) {
            case LOG_RECORD_TIMESTAMP: record_size = sizeof(log_timestamp_t); break;
            case LOG_RECORD_GPS:       record_size = sizeof(log_gps_t); break;
            case LOG_RECORD_IMU:       record_size = sizeof(log_imu_t); break;
            case LOG_RECORD_SERVO:     record_size = sizeof(log_servo_t); break;
            default:
                fprintf(stderr, "Unknown record id 0x%02X at block %" PRIu32
                        " offset %zu; cannot safely resynchronise.\n", id, b, pos);
                return -1;
            }

            if (record_size > BLOCK_SIZE - pos) {
                fprintf(stderr, "Truncated record at end of block %u.\n", b);
                break;
            }

            if (print_enable) print_record(block + pos, id);
            if (csv) csv_record(csv, block + pos, id);
            pos += record_size;
        }

        if (b == g_flight.end_block)
            break;
    }
    return 0;
}

static int dump_binary(const char *filename)
{
    FILE *out = fopen(filename, "wb");
    if (!out) { perror("fopen binary output"); return -1; }

    uint8_t block[BLOCK_SIZE];
    for (uint32_t b = g_flight.start_block; ; ++b) {
        if (read_block(b, block) != 0 ||
            fwrite(block, 1, BLOCK_SIZE, out) != BLOCK_SIZE) {
            perror("Writing binary output");
            fclose(out);
            return -1;
        }
        if (b == g_flight.end_block) break;
    }
    fclose(out);
    return 0;
}

int INITIALIZE_SD_CARD(const char *PATH,
                       bool ENABLE_BIN_FILE, const char *BIN_FILENAME,
                       bool ENABLE_CSV_FILE, const char *CSV_FILENAME,
                       bool PRINT_ENABLE)
{
    g_fd = open(PATH, O_RDONLY);
    if (g_fd < 0) { perror("Failed to open SD image/device"); return 1; }

    int rc = 1;
    if (load_superblock() != 0) goto done;

    if (g_sb.curr_flight_num_rel == 0) {
        printf("No completed flights recorded on the SD card.\n");
        goto done;
    }

    printf("Completed flights: %u\n", g_sb.curr_flight_num_rel);
    printf("Enter absolute flight number: ");
    uint32_t chosen;
    if (scanf("%" SCNu32, &chosen) != 1) {
        fprintf(stderr, "Invalid flight number.\n");
        goto done;
    }
    if (find_flight(chosen) != 0) {
        fprintf(stderr, "Flight %u was not found in metadata.\n", chosen);
        goto done;
    }

    printf("Flight %u: start block=%u, end block=%" PRIu32
           ", start_ms=%u, end_ms=%u\n",
           g_flight.flight_num_abs, g_flight.start_block, g_flight.end_block,
           g_flight.start_ms, g_flight.end_ms);
    printf("GPS start position: lat=%d, lon=%d\n",
           g_flight.lat, g_flight.lon);

    if (ENABLE_BIN_FILE) {
        if (!BIN_FILENAME || dump_binary(BIN_FILENAME) != 0) goto done;
        printf("Binary dump written to %s\n", BIN_FILENAME);
    }
    if (ENABLE_CSV_FILE) {
        if (!CSV_FILENAME) { fprintf(stderr, "Missing CSV filename.\n"); goto done; }
        FILE *csv = fopen(CSV_FILENAME, "w");
        if (!csv) { perror("fopen CSV output"); goto done; }
        fprintf(csv, "type,timestamp_or_value_1,value_2,value_3,value_4,value_5,value_6,value_7,value_8,value_9,value_10,value_11\n");
        int prc = process_flight(csv, PRINT_ENABLE);
        fclose(csv);
        if (prc != 0) goto done;
        printf("CSV written to %s\n", CSV_FILENAME);
    } else if (PRINT_ENABLE) {
        if (process_flight(NULL, 1) != 0) goto done;
    }

    rc = 0;
done:
    close(g_fd);
    g_fd = -1;
    return rc;
}
