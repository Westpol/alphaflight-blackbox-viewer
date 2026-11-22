#ifndef LOGGING_STRUCTS_H
#define LOGGING_STRUCTS_H

#include <stdint.h>
#include <stdio.h>

/* Record IDs and packed layouts copied from the FC logger. */
enum {
    LOG_RECORD_TIMESTAMP = 0x01,
    LOG_RECORD_GPS       = 0x02,
    LOG_RECORD_IMU       = 0x03,
    LOG_RECORD_SERVO     = 0x04
};

typedef struct __attribute__((packed)) {
    uint8_t id;
    uint32_t millis;
} log_timestamp_t;

typedef struct __attribute__((packed)) {
    uint8_t id;
    uint8_t numSV;
    uint8_t padding[2];
    uint32_t timestamp;
    int32_t lat;
    int32_t lon;
    int32_t speed;
    int32_t height;
    int32_t course;
} log_gps_t;

typedef struct __attribute__((packed)) {
    uint8_t id;
    uint8_t padding;
    int16_t wx, wy, wz;
    int16_t accx, accy, accz;
    int16_t quat_w, quat_x, quat_y, quat_z;
    int16_t pitch, roll;
} log_imu_t;

typedef struct __attribute__((packed)) {
    uint8_t id;
    uint8_t padding;
    uint16_t pwm[4];
    uint16_t dshot;
} log_servo_t;

#endif
