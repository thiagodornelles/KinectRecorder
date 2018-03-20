
/*
-- Georgia Tech 2016 Spring
--
-- This is a sample code to show how to use the libfreenet2 with OpenCV
--
-- The code will streams RGB, IR and Depth images from an Kinect sensor.
-- To use multiple Kinect sensor, simply initial other "listener" and "frames"

-- This code refered from sample code provided from libfreenet2: Protonect.cpp
-- https://github.com/OpenKinect/libfreenect2
-- and another discussion from: http://answers.opencv.org/question/76468/opencvkinect-onekinect-for-windows-v2linuxlibfreenect2/


-- Contact: Chih-Yao Ma at <cyma@gatech.edu>
*/

//! [headers]
#include <iostream>
#include <stdio.h>
#include <iomanip>
#include <time.h>
#include <signal.h>
#include <opencv2/opencv.hpp>

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>
//! [headers]

using namespace std;
using namespace cv;

unsigned int frameCount = 0;
bool recording = false;

bool protonect_shutdown = false; // Whether the running application should shut down.

void sigint_handler(int s)
{
    protonect_shutdown = true;
}

void removeDepthWithoutColor(cv::Mat &depth, const cv::Mat &rgb){
    uchar r, g, b;
    for (int i = 0; i < rgb.rows; ++i){
        int32_t* pixel = rgb.ptr<int32_t>(i);  // point to first color in row
        int16_t* pixelDepth = depth.ptr<int16_t>(i);
        for (int j = 0; j < rgb.cols; ++j){
            if(*pixel == 0){
                *pixelDepth = 0;
            }
            pixel++;
            pixelDepth++;
        }

    }
}

void DepthImage_convert_32FC1_to_16UC1(cv::Mat &dest, const cv::Mat &src, float scale) {
    //    assert(src.type() != CV_32FC1 && "DepthImage_convert_32FC1_to_16UC1: source image of different type from 32FC1");
    float *sptr = (float*)src.data;
    int size = src.rows * src.cols;
    float *send = sptr + size;
    dest.create(src.rows, src.cols, CV_16UC1);
    dest.setTo(cv::Scalar(0));
    unsigned short *dptr = (unsigned short*)dest.data;
    while(sptr<send) {
        if(*sptr < std::numeric_limits<float>::max())
            *dptr = scale * (*sptr);
        dptr ++;
        sptr ++;
    }
}

void DepthImage_convert_16UC1_to_32FC1(cv::Mat &dest, const cv::Mat &src, float scale) {
    //    assert(src.type() != CV_16UC1 && "DepthImage_convert_16UC1_to_32FC1: source image of different type from 16UC1");
    const unsigned short *sptr = (const unsigned short*)src.data;
    int size = src.rows * src.cols;
    const unsigned short *send = sptr + size;
    dest.create(src.rows, src.cols, CV_32FC1);
    dest.setTo(cv::Scalar(0.0f));
    float *dptr = (float*)dest.data;
    while(sptr < send) {
        if(*sptr)
            *dptr = scale * (*sptr);
        dptr ++;
        sptr ++;
    }
}

int main()
{

    long maxDistance = 2000;
    system("rm -rf data/depth");
    system("rm -rf data/rgb");
    system("mkdir data/depth");
    system("mkdir data/rgb");
    std::cout << "Streaming from Kinect One sensor!" << std::endl;

    //! [context]
    libfreenect2::Freenect2 freenect2;
    libfreenect2::Freenect2Device *dev = 0;
    libfreenect2::PacketPipeline *pipeline = new libfreenect2::OpenGLPacketPipeline();
    //! [context]

    //! [discovery]
    if(freenect2.enumerateDevices() == 0)
    {
        std::cout << "no device connected!" << std::endl;
        return -1;
    }

    string serial = freenect2.getDefaultDeviceSerialNumber();

    std::cout << "SERIAL: " << serial << std::endl;

    if(pipeline)
    {
        //! [open]
        dev = freenect2.openDevice(serial, pipeline);
        //! [open]
    } else {
        dev = freenect2.openDevice(serial);
    }

    if(dev == 0)
    {
        std::cout << "failure opening device!" << std::endl;
        return -1;
    }

    signal(SIGINT, sigint_handler);
    protonect_shutdown = false;

    //! [listeners]
    libfreenect2::SyncMultiFrameListener listener(libfreenect2::Frame::Color |
                                                  libfreenect2::Frame::Depth |
                                                  libfreenect2::Frame::Ir);
    libfreenect2::FrameMap frames;

    dev->setColorFrameListener(&listener);
    dev->setIrAndDepthFrameListener(&listener);

    //! [listeners]

    //! [start]
    dev->start();

    std::cout << "device serial: " << dev->getSerialNumber() << std::endl;
    std::cout << "device firmware: " << dev->getFirmwareVersion() << std::endl;
    //! [start]

    //! [registration setup]
    libfreenect2::Registration* registration = new libfreenect2::Registration(dev->getIrCameraParams(), dev->getColorCameraParams());
    libfreenect2::Frame undistorted(512, 424, 4), registered(512, 424, 4), depth2rgb(1920, 1080 + 2, 4); // check here (https://github.com/OpenKinect/libfreenect2/issues/337) and here (https://github.com/OpenKinect/libfreenect2/issues/464) why depth2rgb image should be bigger
    //! [registration setup]

    Mat rgbmat, depthmat, depthmatUndistorted, irmat, rgbd;

    std::cerr << "Color calibration\n";
    std::cerr << "cx: " << dev->getColorCameraParams().cx << std::endl;
    std::cerr << "cy: " << dev->getColorCameraParams().cy << std::endl;
    std::cerr << "fx: " << dev->getColorCameraParams().fx << std::endl;
    std::cerr << "fy: " << dev->getColorCameraParams().fy << std::endl;
    std::cerr << "IR calibration\n";
    std::cerr << "cx: " << dev->getIrCameraParams().cx << std::endl;
    std::cerr << "cy: " << dev->getIrCameraParams().cy << std::endl;
    std::cerr << "fx: " << dev->getIrCameraParams().fx << std::endl;
    std::cerr << "fy: " << dev->getIrCameraParams().fy << std::endl;

    //! [loop start]
    while(!protonect_shutdown)
    {
        listener.waitForNewFrame(frames);
        libfreenect2::Frame *rgb = frames[libfreenect2::Frame::Color];
        libfreenect2::Frame *ir = frames[libfreenect2::Frame::Ir];
        libfreenect2::Frame *depth = frames[libfreenect2::Frame::Depth];
        //! [loop start]
        //        cv::Mat(rgb->height, rgb->width, CV_8UC4, rgb->data).copyTo(rgbmat);
        //        cv::Mat(ir->height, ir->width, CV_32FC1, ir->data).copyTo(irmat);
        //        cv::Mat(depth->height, depth->width, CV_32FC1, depth->data).copyTo(depthmat);
        //        cv::imshow("rgb", rgbmat);
        //        cv::imshow("ir", irmat / 4096.0f);
        //        cv::imshow("depth", depth);

        //! [registration]
        registration->apply(rgb, depth, &undistorted, &registered, true);

        //! [registration]
        cv::Mat(undistorted.height, undistorted.width, CV_32FC1, undistorted.data).copyTo(depthmatUndistorted);
        cv::Mat(registered.height, registered.width, CV_8UC4, registered.data).copyTo(rgbd);
//        resize(depthmatUndistorted, depthmatUndistorted, Size(depthmatUndistorted.cols/2, depthmatUndistorted.rows/2));
//        resize(rgbd, rgbd, Size(rgbd.cols/2, rgbd.rows/2));
        //        cv::Mat(rgb->height, rgb->width, CV_8UC4, rgb->data).copyTo(rgbmat);
        //        cv::Mat(depth2rgb.height, depth2rgb.width, CV_32FC1, depth2rgb.data).copyTo(rgbd2);
        Mat depth16;
        threshold(depthmatUndistorted, depthmatUndistorted, maxDistance, 6000, 4);
        DepthImage_convert_32FC1_to_16UC1(depth16, depthmatUndistorted, 1.0f);        
        removeDepthWithoutColor(depth16, rgbd);

        flip(depth16, depth16, 1);
        flip(rgbd, rgbd, 1);


        cv::imshow("undistorted", depth16);
        cv::imshow("registered", rgbd);
        //        cv::imshow("color", rgbmat);


        if (recording) {
            std::stringstream ss;
            ss << "./data/depth/";
            ss << setfill('0') << setw(7) << frameCount;
            ss << ".png";
            //            std::cerr << "Gravando " << ss.str() << "\n";
            //            std::vector<int> compression_params;
            //            compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
            //            compression_params.push_back(0);
            //            cv::imwrite(ss.str(), depthmatUndistorted, compression_params);
            cv::imwrite(ss.str(), depth16);

            //limpar o ss
            ss.str(std::string());
            ss.clear();

            ss << "./data/rgb/";
            ss << setfill('0') << setw(7) << frameCount;
            ss << ".png";
            //            std::cerr << "Gravando " << ss.str() << "\n";
            cv::imwrite(ss.str(), rgbd);
            frameCount++;
        }
        //        cv::imshow("depth2RGB", rgbd2 / 4096.0f);

        char key = cv::waitKey(1);
        protonect_shutdown = protonect_shutdown || key == 'q';

        if(key == 'r' || key == 'R'){
            recording = !recording;
        }

        if(key == '-'){
            maxDistance -= 10;
            cerr << maxDistance << endl;
        }
        if(key == '='){
            maxDistance += 10;
            cerr << maxDistance << endl;
        }

        //! [loop end]
        listener.release(frames);
    }
    //! [loop end]

    //! [stop]
    dev->stop();
    dev->close();
    //! [stop]

    delete registration;

    std::cout << "Streaming Ends!" << std::endl;
    return 0;
}
