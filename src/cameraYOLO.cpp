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


