#pragma once
//#include "ir_sensor.h"
#include <openvino/openvino.hpp>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <vector>
#include <string>
#include <iostream>

#ifndef SIM
//camera?            
#endif

using namespace cv;
using namespace tesseract;
using namespace cv::dnn;
using namespace std;

const float INPUT_WIDTH = 640.0;
const float INPUT_HEIGHT = 640.0;
const float SCORE_THRESHOLD = 0.75; 
const float NMS_THRESHOLD = 0.45; 

void initCameraSystem();
void stopCameraSystem();
string getLicencePlate(Mat& frame, Rect& box, TessBaseAPI* api);
