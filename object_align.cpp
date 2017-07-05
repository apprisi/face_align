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

#include "object_align.h"


#define QT_FIX_POINT_Q 14

static void QT_resize_gray_image(uint8_t *src, int srcw, int srch, int srcs, uint8_t *dst, int dstw, int dsth, int dsts){
    uint16_t *xtable = NULL;

    uint16_t FIX_0_5 = 1 << (QT_FIX_POINT_Q - 1);
    float scalex, scaley;

    scalex = srcw / float(dstw);
    scaley = srch / float(dsth);

    xtable = new uint16_t[dstw * 2];

    for(int x = 0; x < dstw; x++){
        float xc = x * scalex;

        if(xc < 0) xc = 0;
        if(xc >= srcw - 1) xc = srcw - 1.01f;

        int x0 = int(xc);

        xtable[x * 2] = x0;
        xtable[x * 2 + 1] = (1 << QT_FIX_POINT_Q) - (xc - x0) * (1 << QT_FIX_POINT_Q);
    }

    int sId = 0, dId = 0;

    for(int y = 0; y < dsth; y++){
        int x;
        float yc;

        uint16_t wy0;
        uint16_t y0, y1;
        uint16_t *ptrTab = xtable;

        yc = y * scaley;

        if(yc < 0) yc = 0;
        if(yc >= srch - 1) yc = srch - 1.01f;

        y0 = uint16_t(yc);
        y1 = y0 + 1;

        wy0 = (1 << QT_FIX_POINT_Q) - uint16_t((yc - y0) * (1 << QT_FIX_POINT_Q));

        sId = y0 * srcs;

        uint8_t *ptrDst = dst + dId;


        for(x = 0; x < dstw; x++){
            uint16_t x0, x1, wx0, vy0, vy1;

            uint8_t *ptrSrc0, *ptrSrc1;
            x0 = ptrTab[0], x1 = x0 + 1;
            wx0 = ptrTab[1];

            ptrSrc0 = src + sId + x0;
            ptrSrc1 = src + sId + srcs + x0;

            vy0 = (wx0 * (ptrSrc0[0] - ptrSrc0[1]) + (ptrSrc0[1] << QT_FIX_POINT_Q) + FIX_0_5) >> QT_FIX_POINT_Q;
            vy1 = (wx0 * (ptrSrc1[0] - ptrSrc1[1]) + (ptrSrc1[1] << QT_FIX_POINT_Q) + FIX_0_5) >> QT_FIX_POINT_Q;

            dst[y * dsts + x] = (wy0 * (vy0 - vy1) + (vy1 << QT_FIX_POINT_Q) + FIX_0_5) >> QT_FIX_POINT_Q;

            ptrTab += 2;
        }

        dId += dsts;
    }

    delete [] xtable;
}


static uint8_t* mean_filter_3x3_res(uint8_t *img, int width, int height, int stride){

    uint8_t *buffer = new uint8_t[stride * height];

    for(int y = 1; y < height - 1; y++){
        for(int x = 1; x < width - 1; x++){
            uint8_t *ptr = img + y * stride + x;

            uint32_t value = ptr[-stride - 1] + ptr[-stride] + ptr[-stride + 1] +
                             ptr[-1] + ptr[0] + ptr[1] +
                             ptr[ stride - 1] + ptr[ stride] + ptr[stride + 1];

            buffer[y * width + x] = value * 0.111111f;
        }
    }

    return buffer;
}


static void similarity_transform(Shape &src, Shape &dst, SimiArgs &arg){
    int ptsSize = src.ptsSize;
    float cx1 = 0.0f, cy1 = 0.0f;
    float cx2 = 0.0f, cy2 = 0.0f;

    int i;

    for(i = 0; i < ptsSize; i++){
        cx1 += src.pts[i].x;
        cy1 += src.pts[i].y;
    }

    cx1 /= ptsSize;
    cy1 /= ptsSize;


    for(i = 0; i < ptsSize; i++){
        cx2 += dst.pts[i].x;
        cy2 += dst.pts[i].y;
    }

    cx2 /= ptsSize;
    cy2 /= ptsSize;

    float ssina = 0.0f, scosa = 0.0f, num = 0.0f;
    float var1 = 0.0f, var2 = 0.0f;

    for(int i = 0; i < ptsSize; i++){
        float sx = src.pts[i].x - cx1;
        float sy = src.pts[i].y - cy1;

        float dx = dst.pts[i].x - cx2;
        float dy = dst.pts[i].y - cy2;

        var1 += sqrtf(sx * sx + sy * sy);
        var2 += sqrtf(dx * dx + dy * dy);

        ssina += (sy * dx - sx * dy);
        scosa += (sx * dx + sy * dy);
    }

    num = 1.0f / sqrtf(ssina * ssina + scosa * scosa);

    arg.sina = ssina * num;
    arg.cosa = scosa * num;

    arg.scale = var2 / var1;

    arg.cen1.x = cx1;
    arg.cen1.y = cy1;
    arg.cen2.x = cx2;
    arg.cen2.y = cy2;

    arg.ssina = arg.scale * arg.sina;
    arg.scosa = arg.scale * arg.cosa;
}


static int pix_difference_feature(uint8_t *img, int stride, Shape &shape,
        FeatElem &featType, SimiArgs &arg)
{
    int a, b;
    int x, y;

    QTPoint point;

    //point 1
    point = shape.pts[featType.pIdx1];

    x = featType.off1X * arg.scosa + featType.off1Y * arg.ssina + point.x;
    y = featType.off1Y * arg.scosa - featType.off1X * arg.ssina + point.y;

    a = img[y * stride + x];

    //point 2
    point = shape.pts[featType.pIdx2];

    x = featType.off2X * arg.scosa + featType.off2Y * arg.ssina + point.x;
    y = featType.off2Y * arg.scosa - featType.off2X * arg.ssina + point.y;

    b = img[y * stride + x];

    return a - b;
}


uint8_t predict(RegressTree *root, uint8_t *img, int stride, Shape &curShape, SimiArgs &arg){
    root = root->lchild + (pix_difference_feature(img, stride, curShape, root->featType, arg) > root->thresh);
    root = root->lchild + (pix_difference_feature(img, stride, curShape, root->featType, arg) > root->thresh);
    root = root->lchild + (pix_difference_feature(img, stride, curShape, root->featType, arg) > root->thresh);

    return root->leafID;

}


void load(FILE *fin, int depth, RegressTree *root){
    assert(fin != NULL && root != NULL);

    int nlSize = (1 << depth) - 1;
    int leafSize = 1 << (depth - 1);
    int nodeSize = nlSize - leafSize;
    int ret;

    for(int i = 0; i < nodeSize; i++){
        RegressNode *node = root + i;
        FeatElem ft;

        int16_t value;

        ret = fread(&ft.pIdx1, sizeof(uint8_t), 1, fin); assert(ret == 1);
        ret = fread(&ft.pIdx2, sizeof(uint8_t), 1, fin); assert(ret == 1);

        ret = fread(&value, sizeof(int16_t), 1, fin); assert(ret == 1);
        ft.off1X = value * 0.01f;

        ret = fread(&value, sizeof(int16_t), 1, fin); assert(ret == 1);
        ft.off1Y = value * 0.01f;

        ret = fread(&value, sizeof(int16_t), 1, fin); assert(ret == 1);
        ft.off2X = value * 0.01f;

        ret = fread(&value, sizeof(int16_t), 1, fin); assert(ret == 1);
        ft.off2Y = value * 0.01f;

        ret = fread(&node->thresh, sizeof(int16_t), 1, fin);
        assert(ret == 1);

        node->featType = ft;
        node->lchild = root + i * 2 + 1;
        node->rchild = root + i * 2 + 2;
    }

    RegressNode *leafs = root + nodeSize;

    for(int i = 0; i < leafSize; i++){
        leafs[i].leafID = i;
        leafs[i].lchild = NULL;
        leafs[i].rchild = NULL;
    }
}


void release(RegressTree **root){
    if(*root != NULL){
        delete [] *root;
    }

    *root = NULL;
}


void predict(RandomForest *forest, Shape &meanShape, uint8_t *img, int stride, Shape &curShape){
    if(forest->treeSize == 0)
        return ;

    SimiArgs arg;
    uint8_t leafIDs[1024];
    int ptsSize = curShape.ptsSize;
    int ptsSize2 = curShape.ptsSize << 1;

    similarity_transform(meanShape, curShape, arg);

    for(int i = 0; i < forest->treeSize; i ++)
        leafIDs[i] = predict(forest->trees[i], img, stride, curShape, arg);

    Shape delta;

    memset(&delta, 0, sizeof(Shape));
    delta.ptsSize = curShape.ptsSize;

    for(int i = 0; i < forest->treeSize; i++){
        float *ptr = forest->residuals[i] + leafIDs[i] * ptsSize2;

        int j;

        for(j = 0; j < ptsSize; j++){
            delta.pts[j].x += ptr[0];
            delta.pts[j].y += ptr[1];

            ptr += 2;
        }
    }

    float sina = arg.ssina;
    float cosa = arg.scosa;

    for(int i = 0; i < ptsSize; i++){
        float x = delta.pts[i].x;
        float y = delta.pts[i].y;

        curShape.pts[i].x += (x * cosa + y * sina);
        curShape.pts[i].y += (y * cosa - x * sina);
    }

    return ;
}


void predict(RandomForest *forest, int size, Shape &meanShape, uint8_t *img, int stride, Shape &shape){
    for(int i = 0; i < size; i++)
        predict(forest + i, meanShape, img, stride, shape);
}


void read_residuals(FILE *fin, float **residuals, int size, int len){
    assert(fin != NULL);
    assert(residuals != NULL);

    uint8_t *buffer = new uint8_t[len];

    for(int i = 0; i < size; i++){
        float *ptr = residuals[i];
        int ret;

        ret = fread(ptr, sizeof(float), len, fin); assert(ret == len);
    }

    delete [] buffer;
}


void load(FILE *fin, int ptsSize, RandomForest *forest){
    int ret;

    ret = fread(&forest->treeSize, sizeof(int), 1, fin);
    assert(ret == 1);

    ret = fread(&forest->depth, sizeof(int), 1, fin);
    assert(ret == 1);

    int leafSize = 1 << (forest->depth - 1);
    int nlSize   = (1 << forest->depth) - 1;
    int len = ptsSize * 2 * leafSize;

    forest->trees = new RegressTree*[forest->treeSize];
    forest->residuals = new float*[forest->treeSize];
    forest->residuals[0] = new float[forest->treeSize * len];

    for(int i = 0; i < forest->treeSize; i++){
        forest->trees[i] = new RegressTree[nlSize];
        forest->residuals[i] = forest->residuals[0] +  i * len;

        load(fin, forest->depth, forest->trees[i]);
    }

    read_residuals(fin, forest->residuals, forest->treeSize, len);
}


void release_data(RandomForest *forest){
    if(forest == NULL)
        return ;

    if(forest->trees != NULL){
        for(int i = 0; i < forest->treeSize; i++)
            release(forest->trees + i);

        delete [] forest->trees;
    }

    forest->trees = NULL;

    if(forest->residuals != NULL){
        if(forest->residuals[0] != NULL)
            delete [] forest->residuals[0];
        forest->residuals[0] = NULL;

        delete [] forest->residuals;
    }

    forest->residuals = NULL;

    forest->capacity = 0;
    forest->treeSize = 0;
}


void release(RandomForest **forest){
    if(*forest != NULL){
        release_data(*forest);
        delete *forest;
    }

    *forest = NULL;
}


void predict(FaceAligner *aligner, uint8_t *img, int stride, Shape &shape){
    shape = aligner->meanShape;

    uint8_t *src = mean_filter_3x3_res(img, aligner->WINW, aligner->WINH, stride);

    for(int i = 0; i < aligner->ssize; i++)
        predict(aligner->forests + i, aligner->meanShape, src, aligner->WINW, shape);

    delete [] src;
}


void predict(FaceAligner *aligner, uint8_t *img, int width, int height, int stride, Shape &shape){
    int WINW = aligner->WINW;
    int WINH = aligner->WINH;

    if(width == WINW && height == WINH){
        predict(aligner, img, stride, shape);
        return ;
    }

    uint8_t *src = new uint8_t[WINW * WINH];

    float scale = width / float(WINW);

    assert(fabs(scale - height / float(WINH)) < 0.0001f);

    QT_resize_gray_image(img, width, height, stride, src, WINW, WINH, WINW);

    predict(aligner, src, WINW, shape);

    for(int i = 0; i < shape.ptsSize; i++){
        shape.pts[i].x *= scale;
        shape.pts[i].y *= scale;
    }

    delete [] src;
}


int load(const char *filePath, FaceAligner **raligner){
    FILE *fin = fopen(filePath, "rb");
    if(fin == NULL){
        printf("Can't open file %s\n", filePath);
        return 1;
    }

    int ret;
    int ptsSize;

    FaceAligner *aligner = new FaceAligner;

    ret = fread(&aligner->meanShape, sizeof(Shape), 1, fin); assert(ret == 1);
    ret = fread(&aligner->WINW, sizeof(int), 1, fin); assert(ret == 1);
    ret = fread(&aligner->WINH, sizeof(int), 1, fin); assert(ret == 1);
    ret = fread(&aligner->ssize, sizeof(int), 1, fin); assert(ret == 1);

    aligner->forests = new RandomForest[aligner->ssize];
    aligner->capacity = aligner->ssize;

    memset(aligner->forests, 0, sizeof(RandomForest) *aligner->ssize);

    ptsSize = aligner->meanShape.ptsSize;

    assert(ptsSize == QT_POINT_SIZE);

    for(int i = 0; i < aligner->ssize; i++)
        load(fin, ptsSize, aligner->forests + i);

    fclose(fin);

    *raligner = aligner;

    return 0;
}


void release_data(FaceAligner *aligner){
    if(aligner != NULL){
        for(int i = 0; i < aligner->ssize; i++)
            release_data(aligner->forests + i);

        delete [] aligner->forests;
    }

    aligner->forests = NULL;
    aligner->capacity = 0;
    aligner->ssize = 0;
}


void release(FaceAligner **aligner){
    if(*aligner != NULL){
        release_data(*aligner);
        delete *aligner;
    }

    *aligner = NULL;
}
