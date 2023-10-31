#include "PostProcessor.h"
#include "DataTypeDMS.h"

PostDMSMTYolox::~PostDMSMTYolox(){

}

bool PostDMSMTYolox::init(const PostProcessor::Config* config){

    return true;
}

void PostDMSMTYolox::run(const NetData* netData, Output* output){

    printf("PostDMSMTYolox run...\n");

    DMSMTYolox* pOutput = (DMSMTYolox*)output;

    

}