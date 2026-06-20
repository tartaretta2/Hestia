#include "cameraYOLO.h"
#include "houseControl.h"
#include <openvino/openvino.hpp>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <vector>
#include <thread>
#include <string>
#include <iostream>


static thread cameraThread;
extern atomic<bool> alarmOn;


string getLicencePlate(Mat& frame, Rect& box, TessBaseAPI* api) {
    Rect imageRect(0, 0, frame.cols, frame.rows);
    Rect plateRect = box & imageRect;
    if (plateRect.width < 10 || plateRect.height < 10) return "";

    Mat plate = frame(plateRect).clone();

    //taglio bande blu (per targhe europee) (leggermente più conservativo per non tagliare pezzi di lettere)
    int edge_w = plate.cols * 0.06; 
    int edge_h = plate.rows * 0.05; 
    plate = plate(Rect(edge_w, edge_h, plate.cols - 2 * edge_w, plate.rows - 2 * edge_h));

    //pre-processing
    resize(plate, plate, Size(), 2.0, 2.0); 
    cvtColor(plate, plate, COLOR_BGR2GRAY);
    Ptr<CLAHE> clahe = createCLAHE(3.0, Size(8, 8));
    clahe->apply(plate, plate);
    
    //binarizzazione: caratteri bianchi su fondo nero
    threshold(plate, plate, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);

    //ricerca contorni (RETR_CCOMP)
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy; //questo conterrà le relazioni padre-figlio
    findContours(plate, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

    Mat cleanMask = Mat::zeros(plate.size(), CV_8UC1);

    for (size_t i = 0; i < contours.size(); i++) {
        // hierarchy[i][3] è l'indice del padre. se è -1 è un contorno esterno
        if (hierarchy[i][3] == -1) { 
            Rect r = boundingRect(contours[i]);
            float aspectRatio = (float)r.height / r.width;

            //filtro geometrico per la lettera E: aspetto verticale, non troppo stretto o largo, e abbastanza alto rispetto alla targa
            if (aspectRatio > 1.0 && aspectRatio < 5.0 && r.height > plate.rows * 0.45) {
                //passando 'hierarchy' e l'indice 'i' OpenCV disegna la lettera e i suoi buchi interni correttamente.
                drawContours(cleanMask, contours, i, Scalar(255), FILLED, LINE_8, hierarchy);
            }
        }
    }

    //inversione finale per tesseract: lettere nere su sfondo bianco
    bitwise_not(cleanMask, cleanMask); 
    int p = 15;
    copyMakeBorder(cleanMask, cleanMask, p, p, p, p, BORDER_CONSTANT, Scalar(255));

    imshow("DEBUG: Maschera Corretta", cleanMask);

    //OCR
    api->SetImage(cleanMask.data, cleanMask.cols, cleanMask.rows, 1, cleanMask.step);
    char *outText = api->GetUTF8Text();
    string rawText(outText);
    delete[] outText;

    string cleanedText = "";
    for (char c : rawText) {
        //scartiamo tutto tranne lettere A-Z e numeri 0-9
        if (isalnum(c)) {
            cleanedText += (char)toupper(c);
        }
    }

    //lunghezza minima 4 caratteri per essere valida
    if (api->MeanTextConf() < 45 || cleanedText.length() < 4) {
        return "";
    }

    return cleanedText;
}

void cameraLoop(){
    cout<<"Initializing Camera System" <<endl;

    ov::Core core;

    auto model = core.read_model("../best_openvino_model/best.xml");
    auto compiled_model = core.compile_model(model, "CPU");
    auto infer_request = compiled_model.create_infer_request();

    auto input_port = compiled_model.input();
    
    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
    api->SetVariable("tessedit_char_whitelist", "0123456789ABCDEFGHJKLMNPRSTVWXYZ");
    api->SetPageSegMode(PSM_SINGLE_LINE);

    if (api->Init(NULL, "eng")) { 
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }

    VideoCapture cap(0);
    if (!cap.isOpened()) return;

    auto output = infer_request.get_output_tensor(0);
    auto shape = output.get_shape();
    cout << "DEBUG YOLO26 - Output Shape: ";
    for (auto s : shape) cout << s << " ";
    cout << endl;

    Mat frame;
    namedWindow("Licence Plate Detector C++", WINDOW_NORMAL);

    while (alarmOn) {
        cap >> frame;
        if (frame.empty()) break;

        Mat blob;
        blob = dnn::blobFromImage(frame, 1.0 / 255.0, Size(640, 640), Scalar(), true, false);

        ov::Tensor input_tensor(ov::element::f32, {1, 3, 640, 640}, blob.data);
        infer_request.set_input_tensor(input_tensor);

        infer_request.infer();

        auto output = infer_request.get_output_tensor(0);
        float* output_data = output.data<float>();

        Mat detectionMat(300, 6, CV_32F, output_data);
        

        vector<float> confidences;
        vector<Rect> boxes;

        float x_factor = frame.cols / INPUT_WIDTH;
        float y_factor = frame.rows / INPUT_HEIGHT;

        
        for (int i = 0; i < detectionMat.rows; i++) {
            
            float confidence = detectionMat.at<float>(i, 4); 

            if (confidence >= SCORE_THRESHOLD) {


                float x1 = detectionMat.at<float>(i, 0);
                float y1 = detectionMat.at<float>(i, 1);
                float x2 = detectionMat.at<float>(i, 2);
                float y2 = detectionMat.at<float>(i, 3);

                int left = int(x1 * x_factor);
                int top = int(y1 * y_factor);
                int width = int((x2 - x1) * x_factor);
                int height = int((y2 - y1) * y_factor);

                cout << "YOLO26 ha trovato una targa! Conf: " << confidence 
                << " Box: [" << left << "," << top << "," << width << "," << height << "]" << endl;

                boxes.push_back(Rect(left, top, width, height));
                confidences.push_back(confidence);
            }
        }

        vector<int> indices;
        NMSBoxes(boxes, confidences, SCORE_THRESHOLD, NMS_THRESHOLD, indices);
        
        for (int idx : indices) {

            Rect& fineBox = boxes[idx];
            
            string plateText = getLicencePlate(frame, fineBox, api);
            
            rectangle(frame, boxes[idx], Scalar(0, 255, 0), 3);
            
            if(plateText == ""){
                cout <<"No text detected"<<endl;
            }
            else{
                cout <<"Licence plate detected: " <<plateText <<endl;
                checkPlate(plateText);
                putText(frame, plateText, Point(fineBox.x, fineBox.y - 35), FONT_HERSHEY_SIMPLEX, 0.9, Scalar(255, 255, 255), 2);
            }
            
            string label = "TARGA: " + to_string(confidences[idx]).substr(0, 4);
            putText(frame, label, Point(boxes[idx].x, boxes[idx].y - 10), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 1);
        }


        imshow("Licence Plate Detector C++", frame);
        if (waitKey(1) == 'q') break;
    }

    api->End();
    return;
}


void initCameraSystem() {
    cameraThread = thread(cameraLoop);
}

void stopCameraSystem() {
    if (cameraThread.joinable()) {
        cameraThread.join();
    }
}
