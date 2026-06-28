#include "cameraYOLO.h"
#include "houseControl.h"
#include <thread>
#include <string>
#include <iostream>
#include <atomic>

#ifndef SIM
#include <openvino/openvino.hpp>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <vector>
#endif

//thread running the camera loop (real detection or simulation)
static thread cameraThread;

//shared alarm flag defined in main.cpp (the camera runs only while armed)
extern atomic<bool> alarmOn;

#ifndef SIM

//extracts the licence plate text from a detected bounding box and applies OCR
// returns the cleaned plate string or "" if nothing reliable is read
string getLicencePlate(Mat& frame, Rect& box, TessBaseAPI* api) {

    //clamp the detection box to the image bounds to avoid out of range crops
    Rect imageRect(0, 0, frame.cols, frame.rows);
    Rect plateRect = box & imageRect;
    if (plateRect.width < 10 || plateRect.height < 10) return "";

    Mat plate = frame(plateRect).clone();

    //trim the blue side bands (european plates) slightly conservative so we do not cut off parts of the letters
    int edge_w = plate.cols * 0.06; 
    int edge_h = plate.rows * 0.05; 
    plate = plate(Rect(edge_w, edge_h, plate.cols - 2 * edge_w, plate.rows - 2 * edge_h));

    //pre-processing: upscale, grayscale and local contrast equalization (CLAHE)
    resize(plate, plate, Size(), 2.0, 2.0); 
    cvtColor(plate, plate, COLOR_BGR2GRAY);
    Ptr<CLAHE> clahe = createCLAHE(3.0, Size(8, 8));
    clahe->apply(plate, plate);
    
    //binarization (white characters on black background) with otsu's automatic threshold selection
    threshold(plate, plate, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);

    //contour search (RETR_CCOMP keeps a two-level parent/child hierarchy)
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy; // holds the parent-child relationships
    findContours(plate, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

    Mat cleanMask = Mat::zeros(plate.size(), CV_8UC1);

    for (size_t i = 0; i < contours.size(); i++) {
        //hierarchy[i][3] is the parent index (if -1 it is outer contour)
        if (hierarchy[i][3] == -1) { 
            Rect r = boundingRect(contours[i]);
            float aspectRatio = (float)r.height / r.width;

            //geometric filter for characters: vertical aspect neither too narrow nor too wide and tall enough relative to the plate height
            if (aspectRatio > 1.0 && aspectRatio < 5.0 && r.height > plate.rows * 0.45) {
                //passing 'hierarchy' and index 'i' lets OpenCV draw the letter together with its inner holes correctly
                drawContours(cleanMask, contours, i, Scalar(255), FILLED, LINE_8, hierarchy);
            }
        }
    }

    //final inversion for tesseract (black letters on white background)
    bitwise_not(cleanMask, cleanMask); 
    //add a white border so characters are not glued to the image edge
    int p = 15;
    copyMakeBorder(cleanMask, cleanMask, p, p, p, p, BORDER_CONSTANT, Scalar(255));

    //UNCOMMENT TO DEBUG CAMERA
    // imshow("DEBUG: Maschera Corretta", cleanMask);

    // OCR
    api->SetImage(cleanMask.data, cleanMask.cols, cleanMask.rows, 1, cleanMask.step);
    char *outText = api->GetUTF8Text();
    string rawText(outText);
    delete[] outText;

    string cleanedText = "";
    for (char c : rawText) {
        //discard everything except letters A-Z and digits 0-9
        if (isalnum(c)) {
            cleanedText += (char)toupper(c);
        }
    }

    //require a minimum confidence and at least 4 characters to be considered valid
    if (api->MeanTextConf() < 45 || cleanedText.length() < 4) {
        return "";
    }

    return cleanedText;
}

//main camera loop: grabs frames, runs YOLO detection, OCRs each plate and forwards the recognized text to checkPlate()
//runs only while the alarm is armed
void cameraLoop(){
    cout<<"Initializing Camera System" <<endl;

    //load and compile the detection model on CPU
    ov::Core core;

    auto model = core.read_model("../models/best.xml");
    auto compiled_model = core.compile_model(model, "CPU");
    auto infer_request = compiled_model.create_infer_request();

    auto input_port = compiled_model.input();
    
    //initialize tesseract: restrict the character set to plate characters
    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
    api->SetVariable("tessedit_char_whitelist", "0123456789ABCDEFGHJKLMNPRSTVWXYZ");
    api->SetPageSegMode(PSM_SINGLE_LINE);

    if (api->Init(NULL, "eng")) { 
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }

    //open the default camera (device 0)
    VideoCapture cap(0);
    if (!cap.isOpened()) return;

    //UNCOMMENT TO DEBUG CAMERA
    // auto output = infer_request.get_output_tensor(0);
    // auto shape = output.get_shape();
    // cout << "DEBUG YOLO26 - Output Shape: ";
    // for (auto s : shape) cout << s << " ";
    // cout << endl;

    Mat frame;

    //UNCOMMENT TO DEBUG CAMERA
    // namedWindow("Licence Plate Detector C++", WINDOW_NORMAL);

    while (alarmOn) {
        //grab a frame and stop if the stream ended or returned nothing
        cap >> frame;
        if (frame.empty()) break;

        //build the network input blob (normalized, resized to 640x640)
        Mat blob;
        blob = dnn::blobFromImage(frame, 1.0 / 255.0, Size(640, 640), Scalar(), true, false);

        ov::Tensor input_tensor(ov::element::f32, {1, 3, 640, 640}, blob.data);
        infer_request.set_input_tensor(input_tensor);

        //run inference
        infer_request.infer();

        auto output = infer_request.get_output_tensor(0);
        float* output_data = output.data<float>();

        //wrap the raw output as a matrix of detections (rows = candidates)
        Mat detectionMat(300, 6, CV_32F, output_data);
        

        vector<float> confidences;
        vector<Rect> boxes;

        //scale factors to map the 640x640 coordinates back to the frame size
        float x_factor = frame.cols / INPUT_WIDTH;
        float y_factor = frame.rows / INPUT_HEIGHT;

        
        for (int i = 0; i < detectionMat.rows; i++) {
            
            float confidence = detectionMat.at<float>(i, 4); 

            //keep only detections above the score threshold
            if (confidence >= SCORE_THRESHOLD) {


                //corner coordinates as returned by the model
                float x1 = detectionMat.at<float>(i, 0);
                float y1 = detectionMat.at<float>(i, 1);
                float x2 = detectionMat.at<float>(i, 2);
                float y2 = detectionMat.at<float>(i, 3);

                //convert to a pixel-space rectangle in the original frame
                int left = int(x1 * x_factor);
                int top = int(y1 * y_factor);
                int width = int((x2 - x1) * x_factor);
                int height = int((y2 - y1) * y_factor);

                cout << "YOLO26 has found a licence plate! Conf: " << confidence 
                << " Box: [" << left << "," << top << "," << width << "," << height << "]" << endl;

                boxes.push_back(Rect(left, top, width, height));
                confidences.push_back(confidence);
            }
        }

        //non-maximum suppression: drop overlapping duplicate boxes
        vector<int> indices;
        NMSBoxes(boxes, confidences, SCORE_THRESHOLD, NMS_THRESHOLD, indices);
        
        for (int idx : indices) {

            Rect& fineBox = boxes[idx];
            
            // OCR the surviving box
            string plateText = getLicencePlate(frame, fineBox, api);
            
            // draw the detection box on the frame
            //rectangle(frame, boxes[idx], Scalar(0, 255, 0), 3);
            
            if(plateText == ""){
                cout <<"No text detected"<<endl;
            }
            else{
                //valid plate: log it and hand it to the access-control check
                cout <<"Licence plate detected: " <<plateText <<endl;
                checkPlate(plateText);
                putText(frame, plateText, Point(fineBox.x, fineBox.y - 35), FONT_HERSHEY_SIMPLEX, 0.9, Scalar(255, 255, 255), 2);
            }
            
            //overlay the confidence value next to the box
            // string label = "LICENCE PLATE: " + to_string(confidences[idx]).substr(0, 4);
            // putText(frame, label, Point(boxes[idx].x, boxes[idx].y - 10), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 1);
        }

        //UNCOMMENT TO DEBUG CAMERA
        // imshow("Licence Plate Detector C++", frame);
        // if (waitKey(1) == 'q') break;
    }

    //release Tesseract and the camera before leaving the thread
    api->End();
    cap.release();
    return;
}


// simulation mode (no camera)
//after 7s of simulated observation it simulates the recognition of a plate
#else

#include <chrono>

static void cameraLoopSim(){
    cout << "[CAMERA SIM] Initialized. Licence plate recognized in ~7 seconds..." << endl;

    //wait 7s: if the alarm is disarmed meanwhile the thread
    // exits immediately, so the join in stopCameraSystem() never blocks
    for (int i = 0; i < 7 && alarmOn; i++) {
        this_thread::sleep_for(chrono::milliseconds(1000));
    }

    if (!alarmOn) return; // disarmed in the meantime: simulate nothing

    cout << "[CAMERA SIM] Licence plate detected" << endl;
    checkPlate("CZ889KF"); // authorized plate -> requests the disarm
}

#endif

//starts the camera thread (real or simulated depending on the build mode)
void initCameraSystem() {
#ifndef SIM
    cameraThread = thread(cameraLoop);
#else
    cameraThread = thread(cameraLoopSim);
#endif
}

//stops the camera thread, waiting for it to finish
//the loop exits on its own once alarmOn becomes false
void stopCameraSystem() {
    if (cameraThread.joinable()) {
        cameraThread.join();
    }
}
