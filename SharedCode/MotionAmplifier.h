#pragma once

#include "ofMain.h"
#include "ofxCv.h"

class MotionAmplifier {
private:
    cv::Mat rescaled, flow3;
	ofxCv::FlowFarneback flow;
    ofShader shader;
    float scaleFactor;
    ofTexture flowTexture;
    ofVboMesh mesh;
    float rescale;
    
    int stepSize, xSteps, ySteps;
    cv::Mat accumulator;
    bool needToReset;
    
    void duplicateFirstChannel(cv::Mat& twoChannel, cv::Mat& threeChannel) {
        vector<cv::Mat> each;
        cv::split(twoChannel, each);
        each.push_back(each[0]);
        cv::merge(each, threeChannel);
    }
    
public:
    
    float strength, learningRate, blurAmount, windowSize;
    
    MotionAmplifier()
    :strength(0)
    ,learningRate(.9)
    ,blurAmount(3)
    ,windowSize(8) {
    }
    
    void setup(int w, int h, int stepSize, float rescale = 1) {
        this->rescale = rescale;
        shader.load("shaders/MotionAmplifier");
        scaleFactor = 1. / 10; // could dynamically calculate this from flow3
        needToReset = false;
        
        // should switch this to an ofPlanePrimitive
        mesh.setMode(OF_PRIMITIVE_TRIANGLES);
        this->stepSize = stepSize;
        xSteps = 1+((rescale * w) / stepSize);
        ySteps = 1+((rescale * h) / stepSize);
        for(int y = 0; y < ySteps; y++) {
            for(int x = 0; x < xSteps; x++) {
                mesh.addVertex(ofVec2f(x, y) * stepSize / rescale);
            }
        }
        for(int y = 0; y + 1 < ySteps; y++) {
            for(int x = 0; x + 1 < xSteps; x++) {
                int nw = y * xSteps + x;
                int ne = nw + 1;
                int sw = nw + xSteps;
                int se = sw + 1;
                mesh.addIndex(nw);
                mesh.addIndex(ne);
                mesh.addIndex(se);
                mesh.addIndex(nw);
                mesh.addIndex(se);
                mesh.addIndex(sw);
            }
        }
    }
    
    template <class T>
    void update(T& img) {
        ofxCv::resize(img, rescaled, rescale, rescale);
        flow.setWindowSize(windowSize);
		flow.calcOpticalFlow(rescaled);
        duplicateFirstChannel(flow.getFlow(), flow3);
        flow3 *= scaleFactor;
        flow3 += cv::Scalar_<float>(.5, .5, 0);
        if(blurAmount > 0) {
            ofxCv::blur(flow3, blurAmount);
        }
        int w = flow3.cols, h = flow3.rows;
        if(needToReset || accumulator.size() != flow3.size()) {
			needToReset = false;
            ofxCv::copy(flow3, accumulator);
		}
		cv::accumulateWeighted(flow3, accumulator, learningRate);
        // zero the edges
        cv::rectangle(accumulator, cv::Point(0, 0), cv::Point(w-1, h-1), cv::Scalar(.5, .5, 0));
        flowTexture.loadData((float*) accumulator.ptr(), w, h, GL_RGB);
    }
    
    void draw(ofBaseHasTexture& tex) {
        draw(tex.getTexture());
    }
    
    void draw(ofTexture& tex) {
        if(flowTexture.isAllocated()) {
            shader.begin();
            shader.setUniformTexture("source", tex, 1);
            shader.setUniformTexture("flow", flowTexture, 2);
            shader.setUniform1f("strength", strength);
            shader.setUniform1f("scaleFactor", scaleFactor);
            shader.setUniform1f("flowRescale", rescale);
            shader.setUniform1f("sourceRescale", 1);
            mesh.drawFaces();
            shader.end();
        }
    }
    
    void drawMesh() {
        if(flowTexture.isAllocated()) {
            shader.begin();
            shader.setUniformTexture("source", flowTexture, 1);
            shader.setUniformTexture("flow", flowTexture, 2);
            shader.setUniform1f("strength", strength);
            shader.setUniform1f("scaleFactor", scaleFactor);
            shader.setUniform1f("flowRescale", rescale);
            shader.setUniform1f("sourceRescale", rescale);
            mesh.drawWireframe();
            shader.end();
        }
    }
    
    ofTexture& getFlowTexture() {
        return flowTexture;
    }
    
    float getRescale() {
        return rescale;
    }
};