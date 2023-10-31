#pragma once

#include "DataType.h"

class PostProcessor{
public:
    struct Config{
        PostType type;
    };

public:
    virtual ~PostProcessor(){};
    virtual bool init(const Config* config) = 0;
    virtual void run(const NetData* netData, Output* output) = 0;

};

class PostDMSMTFace : public PostProcessor{

public:
    virtual ~PostDMSMTFace();
    virtual bool init(const Config* config);
    virtual void run(const NetData* netData, Output* output);

};

class PostDMSMTYolox : public PostProcessor{

public:
    struct Config : PostProcessor::Config{

    };

public:
    virtual ~PostDMSMTYolox();
    virtual bool init(const PostProcessor::Config* config);
    virtual void run(const NetData* netData, Output* output);

};