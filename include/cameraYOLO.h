#pragma once

#ifndef SIM
#include <openvino/openvino.hpp>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#endif

#include <vector>
#include <string>
#include <iostream>

using namespace std;

#ifndef SIM
using namespace cv;
using namespace tesseract;
using namespace cv::dnn;

// YOLOv26 parameters
const float INPUT_WIDTH = 640.0;
const float INPUT_HEIGHT = 640.0;
const float SCORE_THRESHOLD = 0.75; // Confidence threshold
const float NMS_THRESHOLD = 0.45; // Non-maximum suppression threshold to filter duplicates

string getLicencePlate(Mat& frame, Rect& box, TessBaseAPI* api);
#endif

void initCameraSystem();
void stopCameraSystem();