#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <ncnn/net.h>

#include "QNN/Predictor.h"
#include "QNN/DataTypeDMS.h"

#include "./example/models/ncnn/DFMTN_16f.param.h"
#include "./example/models/ncnn/DFMTN_16f.bin.h"
void test_ncnn(){
    
    std::string testImgPath{"./example/face.jpg"};
    cv::Size inputSize(160, 160);
    std::array<float, 3> mean = std::array<float, 3>{127.5, 127.5, 127.5};
    std::array<float, 3> std = std::array<float, 3>{0.0078125, 0.0078125, 0.0078125};
    std::vector<int> inputNodes{DFMTN_16f_param_id::BLOB_input};
    std::vector<int> outputNodes{DFMTN_16f_param_id::BLOB_lds, DFMTN_16f_param_id::BLOB_euler, DFMTN_16f_param_id::BLOB_cls1, DFMTN_16f_param_id::BLOB_cls2};

    cv::Mat testImg = cv::imread(testImgPath);
    if(testImg.empty()){
        printf("cv::imread fail!(%s)\n", testImgPath.c_str());
        return;
    }
    cv::Mat resizedImg;
    cv::resize(testImg, resizedImg, inputSize);

    ncnn::Net net;
    if((net.load_param(DFMTN_16f_param_bin) == 0) || (net.load_model(DFMTN_16f_bin) == 0)){
        printf("ncnn load fail!\n");
        return;
    }

    // ncnn Mat
    ncnn::Mat input = ncnn::Mat::from_pixels(resizedImg.data, ncnn::Mat::PIXEL_BGR, resizedImg.cols, resizedImg.rows);
    input.substract_mean_normalize(mean.data(), std.data());

    ncnn::Extractor ex = net.create_extractor();
    ex.set_light_mode(true);
    ex.input(inputNodes[0], input);

    std::vector<ncnn::Mat> outputs(outputNodes.size());
    std::vector<std::vector<float>> data(outputNodes.size());
    for(int i = 0; i < outputNodes.size(); i++){
        ex.extract(outputNodes[i], outputs[i]);
        
        float* pData = (float*)(outputs[i].data);
        int dataLen = outputs[i].d * outputs[i].c * outputs[i].h * outputs[i].w;
        for(int j = 0; j < dataLen; j++){
            data[i].push_back(pData[j]);
        }
    }
    
    cv::Mat showImg = testImg.clone();
    // landmark
    for(int i = 0; i < data[0].size(); i+=2){
        cv::Point2f pt(data[0][i], data[0][i+1]);
        pt.x *= testImg.cols;
        pt.y *= testImg.rows;;
        cv::circle(showImg, pt, 1, cv::Scalar(0, 255, 0), -1);
    }

    // euler
    double pitch = data[1][0] * 180 / M_PI;
    double yaw = data[1][1] * 180 / M_PI;
    double roll = data[1][2] * 180 / M_PI;
    printf("pitch: %.2f, yaw: %.2f, roll: %.2f\n", pitch, yaw, roll);

    printf("smoke: %.2f, yawn: %.2f, mask: %.2f\n", data[2][0], data[2][1], data[2][2]);
    printf("ir: %.2f, lefteye_invisible: %.2f, righteye_invisible: %.2f, mouth_invisible: %.2f\n", data[3][0], data[3][1], data[3][2], data[3][3]);

    cv::imwrite("./temp/showImg.jpg", showImg);

    printf("Finish.\n");

}

void test_Predictor(){

    std::string testImgPath{"./example/face.jpg"};

    cv::Mat testImg = cv::imread(testImgPath);
    if(testImg.empty()){
        printf("cv::imread fail!(%s)\n", testImgPath.c_str());
        return;
    }

    PreProcessor::Config preConfig;
    preConfig.tarWidth = 160;
    preConfig.tarHeight = 160;
    preConfig.tarFormat = ImgFormat::FMT_BGR888;

    // Net::Config netConfig;
    NetNCNN::Config netConfig;
    netConfig.type = NetType::NET_NCNN;
    netConfig.mean = std::array<float, 3>{127.5, 127.5, 127.5};
    netConfig.std = std::array<float, 3>{0.0078125, 0.0078125, 0.0078125};
    netConfig.pModelParam = DFMTN_16f_param_bin;
    netConfig.pModelWeight = DFMTN_16f_bin;
    netConfig.inputNodes = std::vector<int>{DFMTN_16f_param_id::BLOB_input};
    netConfig.outputNodes = std::vector<int>{DFMTN_16f_param_id::BLOB_lds, DFMTN_16f_param_id::BLOB_euler, DFMTN_16f_param_id::BLOB_cls1, DFMTN_16f_param_id::BLOB_cls2};
    netConfig.outputShapes = std::vector<NCHW>{
        NCHW{1, 1, 1, 40},
        NCHW{1, 1, 1, 3},
        NCHW{1, 1, 1, 4},
        NCHW{1, 1, 1, 4}
    };

    PostProcessor::Config postConfig;
    postConfig.type = PostType::POST_DMS_MTFACE;

    Predictor predictor;
    Predictor::Config config;
    config.preConfig = &preConfig;
    config.netConfig = &netConfig;
    config.postConfig = &postConfig;

    if(!predictor.init(&config)){
        printf("Predictor init fail!\n");
        return;
    }
    printf("Predictor init successfully!\n");

    Input input;
    input.data = testImg.data;
    input.width = testImg.cols;
    input.height = testImg.rows;
    input.format = ImgFormat::FMT_BGR888;

    DMSMTFace output;
    predictor.run(&input, &output);

    cv::Mat showImg = testImg.clone();
    LandmarkType landmarkType = output.landmark.type;
    switch(landmarkType){
    case LandmarkType::BSJ_20:{
        for(int i = 0; i < 20; i++){
            cv::Point2f pt = output.landmark.points[i];
            pt.x *= testImg.cols;
            pt.y *= testImg.rows;
            cv::circle(showImg, pt, 1, cv::Scalar(0, 255, 0), -1);
        }
        break;
    }
    }

    EulerAngle eulerAngle = output.eulerAngle;
    printf("pitch: %.2f, yaw: %.2f, roll: %.2f\n", eulerAngle.pitch, eulerAngle.yaw, eulerAngle.roll);

    for(int i = 0; i < output.attributes.size(); i++){
        Attribute attribute = output.attributes[i];
        printf("%s: %.2f\n", attribute.label.c_str(), attribute.score);
    }

}

int main(int, char**){
    std::cout << "Hello, from QNN!\n";

    test_ncnn();

    test_Predictor();

    return 0;
}
