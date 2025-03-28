#include <opencv2/opencv.hpp>
#include <iostream>

class CameraService
{
public:
    CameraService()
    {
        std::string pipeline =
            "libcamerasrc ! video/x-raw,width=640,height=480,framerate=2/1,format=RGB ! "
            "videoconvert ! appsink";
        cap.open(pipeline, cv::CAP_GSTREAMER);

        if (!cap.isOpened())
        {
            std::cerr << "Error: Unable to open camera via GStreamer.\n";
            exit(1);
        }

        std::cout << "Camera opened successfully.\n";
    }

    ~CameraService()
    {
        cap.release();
    }

    cv::Mat captureFrame()
    {
        for (int i = 0; i < 5; ++i)
            cap.grab(); // Flush old frames

        cv::Mat frame;
        cap.read(frame);
        return frame;
    }

private:
    cv::VideoCapture cap;
};