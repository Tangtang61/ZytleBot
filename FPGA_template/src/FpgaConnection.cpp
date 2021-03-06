#include <iostream>
#include <chrono>
#include <vector>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <sstream>

// ROS include
#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <std_msgs/String.h>

// openCV include
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

// user
#include "feature.h"
#include "forest.h"
#include "window_candidate.h"
#include "hw.h"


using namespace std;
using namespace cv;

class FpgaConnection
{
    ros::NodeHandle nh_;
    ros::Publisher string_pub_;
    image_transport::ImageTransport it_;
    image_transport::Subscriber image_sub_;
    image_transport::Publisher image_pub_;


public:
    // constructor
    FpgaConnection()
            : it_(nh_) {
        // TODO edit topic name
        image_sub_ = it_.subscribe("/image_raw", 1,
                                   &FpgaConnection::imageCb, this);

        image_pub_ = nh_.advertise<sensor_msgs::ImageConstPtr>("/red_flag", 1);

        red_flag_ = nh_.advertise<std_msgs::String>("/red_flag", 1);

        if(hwmode) hw_setup();
        cout << "hw setup completed" << endl;

        find_flag = false;
        find_count = 0;
    }

    // destructor
    ~ImageConverter()
    {
        cv::destroyAllWindows();
        // TODO release hw
    }

    // callback function
    void imageCb(const sensor_msgs::ImageConstPtr &msg) {
        
        cv_bridge::CvImagePtr cv_ptr, pub_img;
        try {
            // ROS msg format to OpenCV
            cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
        }
        catch (cv_bridge::Exception &e) {
            ROS_ERROR("cv_bridge exception: %s", e.what());
            return;
        }
        cv::Mat base_image = cv_ptr->image;
        // subscribe image clone (not a copy of the address)
        cv::Mat pub_image = base_image.clone();

        // TODO resize image (640*480 example)
        // resize(base_image, base_image, cv::Size(), 640.0/base_image.cols ,480.0/base_image.rows);

        // TODO describe FPGA application

        // publish image
        cv_ptr->image = pub_image;
        image_pub_.publish(cv_ptr->toImageMsg());


        std_msgs::String send_msg;

        // flagの送信処理
        std::stringstream ss;
        if (find_flag) {
            ss << "true";
        } else {
            ss << "false";
        }
        send_msg.data = ss.str();
        red_flag_.publish(send_msg);
    }


    bool inputdatacheck(unsigned char* ptr, cv::Mat img){
        bool rst = true;
        for(int y = 0; y < img.rows; y++){
            for(int x = 0; x < img.cols; x++){
                cv::Vec<unsigned char, 3> pix = img.ptr<cv::Vec3b>(y)[x];
                if(ptr[(y*img.cols+x)*3] != pix[0]){
                    rst = false;
                    cout << y << " " << x << " "  << ptr[(y*img.cols+x)*3] << " " << pix[0];
                }
                if(ptr[(y*img.cols+x)*3+1] != pix[1]){
                    rst = false;
                    cout << y << " " << x << " "  << ptr[(y*img.cols+x)*3+1] << " " << pix[1];
                }
                if(ptr[(y*img.cols+x)*3+2] != pix[2]){
                    rst = false;
                    cout << y << " " << x << " "  << ptr[(y*img.cols+x)*3+2] << " " << pix[2];
                }
            }
        }
        return rst;
    }

    bool inputdatacheck2(unsigned char* ptr, cv::Mat img){
        bool rst = true;
        for(int y = 0; y < img.rows; y++){
            for(int x = 0; x < img.cols; x++){
                int pix = int(img.ptr<uchar>(y)[x]);
                if(ptr[y*img.cols+x] != pix){
                    rst = false;
                    cout << y << " " << x << ptr[y*img.cols+x] << " " << pix << endl;
                    //cout << "unko" << endl;
                }
            }
        }
        if(rst) cout << "input data check2 OK" << endl;
        return rst;
    }

    bool hwresultcheck(unsigned short* sw_feature, unsigned short* hw_feature, int start, int end){
        bool showmode = true;
        bool flg = true;
        for(int i = start; i < end; i++){
            if(((int) sw_feature[i]) != ((int) hw_feature[i])){
                flg = false;
                cout << i << " is wrong , sw: " << int(sw_feature[i]) << " hw: " << int(hw_feature[i]) << endl;
            }
        }
        if(!flg) cout << "hwresult check NG" << endl;
        return flg;
    }

    void test_four_window(float* result, int num, Mat rgb[4], Mat hls[4], Mat gray[4], double* time0, double* time1, double* time2, double *time3){
        std::chrono::system_clock::time_point  t0, t1, t2, t3, t4, t5, t6, t7;
        cv::Size spatial_size(8, 8);
        Mat spatial_rgb[4], spatial_hls[4];
        t0 = std::chrono::system_clock::now();
        for(int i = 0; i < num; i++){
            cv::resize(rgb[i], spatial_rgb[i], spatial_size);
            cv::resize(hls[i], spatial_hls[i], spatial_size);
        }

        if(hwmode){
            memset(hw_feature, 0, sizeof(unsigned short) * FEATURE_SIZE * 4);
            for(int i = 0; i < num; i++){
                ravel(spatial_hls[i], i * FEATURE_SIZE + hw_feature);
                ravel(spatial_rgb[i], i * FEATURE_SIZE + hw_feature + 192);
            }
            t1 = std::chrono::system_clock::now();
            //calculate HOG feature
            t2 = std::chrono::system_clock::now();
            for(int i = 0; i < 32; i++)  memcpy(hog_imageBuffer0 + 64 * i, gray[0].data + gray[0].step * i, gray[0].cols * sizeof(unsigned char));
            if(num > 1) for(int i = 0; i < 32; i++)  memcpy(hog_imageBuffer1 + 64 * i, gray[1].data + gray[1].step * i, gray[1].cols * sizeof(unsigned char));
            if(num > 2) for(int i = 0; i < 32; i++)  memcpy(hog_imageBuffer2 + 64 * i, gray[2].data + gray[2].step * i, gray[2].cols * sizeof(unsigned char));
            if(num > 3) for(int i = 0; i < 32; i++)  memcpy(hog_imageBuffer3 + 64 * i, gray[3].data + gray[3].step * i, gray[3].cols * sizeof(unsigned char));
            calc_hog_hw(num, hw_feature + 192 * 2, hw_feature + 192 * 2 + FEATURE_SIZE, hw_feature + 192 * 2 + FEATURE_SIZE * 2, hw_feature + 192 * 2 + FEATURE_SIZE * 3);
            t3 = std::chrono::system_clock::now();
        }
        //t2 = std::chrono::system_clock::now();
        *time0 += (long double)std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count()/1000;
        *time1 += (long double)std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()/1000;
        *time2 += (long double)std::chrono::duration_cast<std::chrono::microseconds>(t3-t2).count()/1000;
        if(!hwmode || checkmode){
            memset(sw_feature, 0, sizeof(unsigned short) * FEATURE_SIZE * 4);
            for(int i = 0; i < num; i++){
                ravel(spatial_hls[i], i * FEATURE_SIZE + sw_feature);
                ravel(spatial_rgb[i], i * FEATURE_SIZE + sw_feature + 192);
                lite_hog(gray[i], i * FEATURE_SIZE + sw_feature + 192 * 2);
            }
        }
        if(checkmode) hwresultcheck(sw_feature, hw_feature, 0, FEATURE_SIZE * 4);
        //Classify by Random Forest
        t4 = std::chrono::system_clock::now();
        for(int i = 0; i < 4; i++){
            if(hwmode) result[i] = randomforest_classifier(i * FEATURE_SIZE + hw_feature);
            else       result[i] = randomforest_classifier(i * FEATURE_SIZE + sw_feature);
        }
        t5 = std::chrono::system_clock::now();
        *time3 += (long double)std::chrono::duration_cast<std::chrono::microseconds>(t5-t4).count()/1000;
    }

    vector<pair<vector<int>, float>> test_one_frame(Mat frame){
        vector<pair<vector<int>, float>> rst;
        std::chrono::system_clock::time_point  t1, t2, t3, t4, t5, t6, t7;

        t1 = std::chrono::system_clock::now();
        //1. crop use frame
        Mat rgb(frame, cv::Rect(sx_min, sy_min, ex_max - sx_min, ey_max - sy_min));
        //2. convert to HLS image
        Mat hls;
        cv::cvtColor(rgb, hls, CV_BGR2HSV);
        Mat gray;
        cv::cvtColor(rgb, gray, CV_BGR2GRAY);
        t2 = std::chrono::system_clock::now();
        Mat rgb_each_window[4];
        Mat hls_each_window[4];
        Mat gray_each_window[4];
        //3. resize for each window kind
        int cnt = 0;
        for(auto itr = widthkind.begin(); itr != widthkind.end(); ++itr) {
            int width = *itr;
            //TODO:crop use frame
            cv::resize(rgb, rgb_each_window[cnt++], cv::Size(), (float)WINDOW_WIDTH/(float)width , (float)WINDOW_WIDTH/(float)width, INTER_NEAREST);
        }
        cnt = 0;
        for(auto itr = widthkind.begin(); itr != widthkind.end(); ++itr) {
            int width = *itr;
            //TODO:crop use frame
            cv::resize(hls, hls_each_window[cnt++], cv::Size(), (float)WINDOW_WIDTH/(float)width , (float)WINDOW_WIDTH/(float)width, INTER_NEAREST);
        }
        cnt = 0;
        for(auto itr = widthkind.begin(); itr != widthkind.end(); ++itr) {
            int width = *itr;
            //TODO:crop use frame
            cv::resize(gray, gray_each_window[cnt++], cv::Size(), (float)WINDOW_WIDTH/(float)width , (float)WINDOW_WIDTH/(float)width, INTER_NEAREST);
        }
        t3 = std::chrono::system_clock::now();
        //4.crop for each window and classify for each window
        int test_count = 0;
        int widthkind_cnt = 0;
        double time0 = 0;
        double time1 = 0;
        double time2 = 0;
        double time3 = 0;
        for(auto itr = widthkind.begin(); itr != widthkind.end(); ++itr) {
            int width = *itr;
            auto windows = mp[width];
            //process 4 window images for each iteration
            for(int i = 0; i*4 < windows.size(); i++){
                int process_window_num = min(i*4+3, (int)windows.size()-1) - (i*4) + 1; //maximum 4
                Mat rgb_test[4], hls_test[4], gray_test[4];
                for(int j = 0; j < process_window_num; j++){
                    rgb_test[j]  = rgb_each_window[widthkind_cnt](cv::Rect(windows[i*4+j].first, windows[i*4+j].second, WINDOW_WIDTH, WINDOW_HEIGHT));
                    hls_test[j]  = hls_each_window[widthkind_cnt](cv::Rect(windows[i*4+j].first, windows[i*4+j].second, WINDOW_WIDTH, WINDOW_HEIGHT));
                    gray_test[j] = gray_each_window[widthkind_cnt](cv::Rect(windows[i*4+j].first, windows[i*4+j].second, WINDOW_WIDTH, WINDOW_HEIGHT));
                }
                float result[4];
                test_four_window(result, process_window_num, rgb_test, hls_test, gray_test, &time0, &time1, &time2, &time3);
                // imwrite("wind/img" + to_string(test_count) + ".png", rgb_test);
                for(int j = 0; j < process_window_num; j++){
                    if(result[j] >= proba_thresh){
                        int original_window_sx = original_mp[width][i*4+j].first;
                        int original_window_sy = original_mp[width][i*4+j].second;
                        int original_window_ex = original_window_sx + width;
                        int original_window_ey = original_window_sy + width/2;
                        vector<int> coord = {original_window_sx, original_window_sy, original_window_ex, original_window_ey};
                        rst.push_back(make_pair(coord, result[j]));
                    }
                }
                test_count+=process_window_num;
            }
            widthkind_cnt++;
        }
        t4 = std::chrono::system_clock::now();
        cout << "test_count" << test_count << endl;

        double tmp1 = (long double)std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count()/1000;
        double tmp2 = (long double)std::chrono::duration_cast<std::chrono::microseconds>(t3-t2).count()/1000;
        double tmp3 = (long double)std::chrono::duration_cast<std::chrono::microseconds>(t4-t3).count()/1000;
        double all = (long double)std::chrono::duration_cast<std::chrono::microseconds>(t4-t1).count()/1000;
        if(showdetailtime){
            cout << "preprocessing color convert time : " << tmp1 << "[milisec]" << endl;
            cout << "preprocessing resize time : " << tmp2 << "[milisec]" << endl;
            cout << "classify time : " << tmp3 << "[milisec]" << endl;
            cout << "all time : " << all << "[milisec]" << endl;
            cout << "sw ravel time" << time0 << "[milisec]" << endl;
            cout << "hw hist time" << time1 << "[milisec]" << endl;
            cout << "hw hog time" << time2 << "[milisec]" << endl;
            cout << "random forest time" << time3 << "[milisec]" << endl;
        }
        return rst;
    }
};

int main(int argc, char** argv)
{
    ros::init(argc, argv, "FPGA_Connection");
    FpgaConnection fc;
    ros::spin();
    return 0;
}
