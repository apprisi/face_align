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

#ifndef _OBJECT_ALIGN_H_
#define _OBJECT_ALIGN_H_

#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <assert.h>


#define QT_POINT_SIZE 68
#define QT_MAX_POINT_SIZE 68

typedef struct {
    float x;
    float y;
} QTPoint;


typedef struct{
    float sina;
    float cosa;
    float scale;

    QTPoint cen1, cen2;

    float ssina;
    float scosa;
} SimiArgs;


typedef struct {
    QTPoint pts[QT_MAX_POINT_SIZE];
    int ptsSize;
} Shape;



typedef struct FeatElem_t
{
    uint8_t pIdx1, pIdx2;
    float off1X, off1Y;
    float off2X, off2Y;
} FeatElem;


typedef struct RegressNode_t
{
    uint8_t leafID;

    FeatElem featType;
    int16_t thresh;

    struct RegressNode_t *lchild;
    struct RegressNode_t *rchild;
} RegressNode;


typedef RegressNode RegressTree;


typedef struct {
    RegressTree **trees;

    int capacity;
    int treeSize;
    int depth;

    float **residuals;
} RandomForest;


typedef struct {
    Shape meanShape;

    int WINW;
    int WINH;

    RandomForest *forests;
    int ssize;
    int capacity;
} FaceAligner;

void predict(FaceAligner *aligner, uint8_t *img, int width, int height, int stride, Shape &shape);
int load(const char *filePath, FaceAligner **raligner);
void release(FaceAligner **aligner);

#endif
