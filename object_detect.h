/*M///////////////////////////////////////////////////////////////////////////////////////////////////
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//
//    * Using for Personal applications or study are freedom.
//    * Using for Bussiness must ask for agreements.
//
//  Copyright (C) 2017, Hangzhou Qiantu Technology Copyright,(杭州千图科技有限公司) all rights reserved.
//  Author: Zhu xiaoyan
//  email: business@qiantuai.com
//  website: www.qiantuai.com
//M*/

#ifndef _OBJECT_DETECT_H_
#define _OBJECT_DETECT_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <math.h>


typedef struct {
    int x, y;
    int width;
    int height;
} QTRect;



int load_detector(const char *filePath);
void initialize_detect_factor(float startScale, float endScale, float offset, int layer);

int detect_object(uint8_t *img, int width, int height, int stride, QTRect **rects, float **scores);

void release_detector();

#endif
