#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "sdkconfig.h"
#define LV_HOR_RES 320
#define LV_VER_RES 240
#define LV_INDEV_STATE_REL 0
#define LV_INDEV_STATE_PR 1
typedef int lv_indev_state_t;
typedef struct {int unused;} lv_indev_drv_t;
typedef struct {struct {int16_t x,y;} point;lv_indev_state_t state;} lv_indev_data_t;
