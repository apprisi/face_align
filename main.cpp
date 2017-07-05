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


#include "object_detect.h"
#include "object_align.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>



int read_file_list(const char *filePath, std::vector<std::string> &fileList)
{
    char line[512];
    FILE *fin = fopen(filePath, "r");

    if(fin == NULL){
        printf("Can't open file: %s\n", filePath);
        return -1;
    }

    while(fscanf(fin, "%s\n", line) != EOF){
        fileList.push_back(line);
    }

    fclose(fin);

    return 0;
}


void analysis_file_path(const char* filePath, char *rootDir, char *fileName, char *ext)
{
    int len = strlen(filePath);
    int idx = len - 1, idx2 = 0;

    while(idx >= 0){
        if(filePath[idx] == '.')
            break;
        idx--;
    }

    if(idx >= 0){
        strcpy(ext, filePath + idx + 1);
        ext[len - idx] = '\0';
    }
    else {
        ext[0] = '\0';
        idx = len - 1;
    }

    idx2 = idx;
    while(idx2 >= 0){
#ifdef WIN32
        if(filePath[idx2] == '\\')
#else
        if(filePath[idx2] == '/')
#endif
            break;

        idx2 --;
    }

    if(idx2 > 0){
        strncpy(rootDir, filePath, idx2);
        rootDir[idx2] = '\0';
    }
    else{
        rootDir[0] = '.';
        rootDir[1] = '\0';
    }

    strncpy(fileName, filePath + idx2 + 1, idx - idx2 - 1);
    fileName[idx - idx2 - 1] = '\0';
}


void extract_face_from_image(cv::Mat &src, QTRect &rect, cv::Mat &patch, int &dx, int &dy);
float shape_distance(Shape &shapeA, Shape &shapeB);


#define OBJECT_DETECT_MODEL "detect_model.dat"
#define OBJECT_ALIGN_MODEL "align_model.dat"

int main_align(int argc, char **argv){
    if(argc < 3){
        printf("Usage: %s [file list] [out dir]\n", argv[0]);
        return 1;
    }

    FaceAligner *aligner;

    if(load_detector(OBJECT_DETECT_MODEL) != 0){
        printf("Can't open detect model %s\n", OBJECT_DETECT_MODEL);
        return 2;
    }

    if(load(OBJECT_ALIGN_MODEL, &aligner) != 0){
        printf("Can't open align model %s\n", OBJECT_ALIGN_MODEL);
        return 3;
    }

    std::vector<std::string> imgList;
    int size;

    read_file_list(argv[1], imgList);
    size = imgList.size();

    for(int i = 0; i < size; i++){
        char rootDir[128], fileName[128], ext[30];

        const char *imgPath = imgList[i].c_str();

        cv::Mat img = cv::imread(imgPath, 1);

        if(img.empty()){
            printf("Can't open image %s\n", imgPath);
            continue;
        }

        cv::Mat gray;

        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

        QTRect *rects;
        float  *scores;

        int rsize;

        rsize = detect_object(gray.data, gray.cols, gray.rows, gray.step, &rects, &scores);

        printf("rsize = %d\n", rsize);
        for(int r = 0; r < rsize; r++){
            QTRect rect = rects[r];
            cv::Mat patch;
            Shape shape;
            int dx, dy;

            cv::rectangle(img, cv::Rect(rect.x, rect.y, rect.width, rect.height), cv::Scalar(255, 0, 0), 2);

            int cx = rect.x + 0.5 * rect.width;
            int cy = rect.y + 0.5 * rect.height;

            rect.x = cx - 0.83 * rect.width;
            rect.y = cy - 0.83 * rect.height;
            rect.width = rect.width * 1.67;
            rect.height = rect.height * 1.67;

            extract_face_from_image(gray, rect, patch, dx, dy);

            predict(aligner, patch.data, patch.cols, patch.rows, patch.step, shape);

            for(int p = 0; p < shape.ptsSize; p++){
                float x = shape.pts[p].x - dx;
                float y = shape.pts[p].y - dy;

                cv::circle(img, cv::Point2f(x, y), 2, cv::Scalar(0, 255, 0), -1);
            }
        }

        cv::imshow("img", img);
        cv::waitKey();

        analysis_file_path(imgPath, rootDir, fileName,ext);

        if(rsize > 0){
            delete [] rects;
            delete [] scores;
        }
    }

    return 0;
}


int main(int argc, char **argv){
#if defined(MAIN_ALIGN)
    main_align(argc, argv);

#endif
}


float shape_distance(Shape &shapeA, Shape &shapeB){
    int ptsSize = shapeA.ptsSize;

    float sum = 0;
    for(int i = 0; i < ptsSize; i++){
        sum += sqrtf(powf(shapeA.pts[i].x - shapeB.pts[i].x, 2) + powf(shapeA.pts[i].y - shapeB.pts[i].y, 2));
    }

    return sum / ptsSize;
}


void extract_face_from_image(cv::Mat &src, QTRect &rect, cv::Mat &patch, int &dx, int &dy){
    int x0 = rect.x;
    int y0 = rect.y;
    int x1 = rect.x + rect.width  - 1;
    int y1 = rect.y + rect.height - 1;

    int width  = src.cols;
    int height = src.rows;
    int w, h;

    int bl = 0, bt = 0, br = 0, bb = 0;


    if(x0 < 0) {
        bl = -x0;
        x0 = 0;
    }

    if(y0 < 0){
        bt = -y0;
        y0 = 0;
    }

    if(x1 > width - 1){
        br = x1 - width + 1;
        x1 = width - 1;
    }

    if(y1 > height - 1){
        bb = y1 - height + 1;
        y1 = height - 1;
    }

    patch = cv::Mat(rect.height, rect.width, src.type(), cv::Scalar::all(0));

    w = rect.width - bl - br;
    h = rect.height - bt - bb;

    patch(cv::Rect(bl, bt, w, h)) += src(cv::Rect(x0, y0, w, h));

    dx = bl - x0;
    dy = bt - y0;
}

