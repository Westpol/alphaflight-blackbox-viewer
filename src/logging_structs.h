#ifndef LOGGING_STRUCT_H
#define LOGGING_STRUCT_H

#include <stdint.h>
#include <stdio.h>

static struct __attribute__((packed)){
    uint8_t id;
    uint8_t padding;
    int16_t wx;
    int16_t wy;
    int16_t wz;

    int16_t accx;
    int16_t accy;
    int16_t accz;

    int16_t quat_w;
    int16_t quat_x;
    int16_t quat_y;
    int16_t quat_z;

    int16_t pitch;
    int16_t roll;
} imu;

static struct __attribute__((packed)){
    uint8_t id;
    uint8_t numSV;
    uint8_t padding[2];
    uint32_t timestamp;
    int32_t lat;
    int32_t lon;
    int32_t speed;
    int32_t height;
    int32_t course;
} gps;

static struct __attribute__((packed)){
    uint8_t id;
    uint8_t padding;
    uint16_t pwm[4];
    uint16_t dshot;
} servo_motor_out_needed;


typedef struct{
	uint8_t id;
	uint8_t (*decode)(const uint8_t* raw, uint8_t mode, FILE *file);
} DECODER_T;

uint8_t copy_struct_onboard_sensors(const uint8_t* raw, uint8_t mode, FILE *file);

uint8_t copy_struct_crsf(const uint8_t* raw, uint8_t mode, FILE *file);

uint8_t copy_struct_gps(const uint8_t* raw, uint8_t mode, FILE *file);

uint8_t copy_struct_pid(const uint8_t* raw, uint8_t mode, FILE *file);

#endif