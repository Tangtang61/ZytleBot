#include <iostream>
#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "unistd.h"
#include <math.h>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <geometry_msgs/Twist.h>

// pcam使用時
#include "std_msgs/MultiArrayLayout.h"
#include "std_msgs/MultiArrayDimension.h"
#include "std_msgs/UInt8MultiArray.h"


// 最初の交差点もしくはカーブ位置 dirは以下のように0が南で右回り
// map_dataは下で書き換える必要がある
// 2
//1 3
// 0
#define NEXT_X 4
#define NEXT_Y 0
#define START_DIR 3

#define PI 3.141592653589793
#define BIRDSEYE_LENGTH 100
#define BURGER_MAX_LIN_VEL 0.22
#define BURGER_MAX_ANG_VEL 2.84

// 直進の調整
#define STRAIGHT_TOO_RIGHT_LANE 0.25
#define STRAIGHT_TOO_LEFT_LANE 0.05


// カーブの調整
#define RIGHT_CURVE_START_LOST_LINE_TIME 1
#define RIGHT_CURVE_END_MARGIN_TIME 2
#define RIGHT_CURVE_END_TIME 6
#define RIGHT_CURVE_VEL 0.2
#define RIGHT_CURVE_ROT -0.27

#define LEFT_CURVE_END_TIME 2.45
#define LEFT_CURVE_END_MARGIN_TIME 0
#define LEFT_CURVE_VEL 0.1
#define LEFT_CURVE_ROT 0.7
#define LEFT_CURVE_AFTER_ROT 0.1

#define AVOID_OBSTACLE_VEL 0.1
#define AVOID_OBSTACLE_ROT -0.7
#define AVOID_ROT_TIME 2.45
#define AVOID_ROT_STRAIGHT 2.7
#define AVOID_STRAIGHT_TIME 4.75
#define AVOID_BEFORE_STRAIGHT_MARGIN_TIME 0.5

// 一度認識したオブジェクトの位置を推定するための比率
// 大きくすればするほど、後に発見したオブジェクトの優先度が高まる
#define INTERSECTION_PREDICTION_TIME_RATIO 1.5
// カメラの下部から実際に曲がるまでのマージン(s)を決定 TIME_RATIOの影響を受ける
#define INTERSECTION_PREDICTION_UNDER_MARGIN 0.8
// T字路を曲がるときに、Tが画面のどこまで来たら曲がるべきかを調整
#define INTERSECTION_CURVE_START_FLAG_RATIO 0.75

#define OBJECT_DISCARDING_TIME 3


// 走行ラインの左からの割合
// カーブがギリギリの場合大きく
#define RUN_LINE 0.18


#define HUE_L 0
#define HUE_H 180
#define SATURATION_L 0
#define SATURATION_H 45
#define LIGHTNESS_L 220
#define LIGHTNESS_H 255

// zybo_camera用設定
#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480


#define WIDTH_RATIO 0.17
#define HEIGHT_H 0.592
#define HEIGHT_L 0.79

/*
#define WIDTH_RATIO 0.19
#define HEIGHT_H 0.59
#define HEIGHT_L 0.8
*/


typedef struct object {
public:
    // オブジェクトの種類
    // obstacle, intersection, people
    std::string objType;
    int beforeX;
    int beforeY;
    int findCnt;
    ros::Time timeStamp;
} OBJECT;


// 直線を中点、傾き、長さで表す
typedef struct straight {
    cv::Point2f middle;
    float degree;
    float length;
} STRAIGHT;

// map_data[y][x][0]がタイルの種類
// map_data[y][x][1]が向きを表している
// 向きは1がデータ画像のとおりで、0~3で右回りに表現されている
int map_data[7][5][2] = {{{3, 0}, {4, 0}, {7, 2}, {4, 0}, {3, 1}},
                         {{6, 1}, {0, 0}, {1, 1}, {0, 0}, {6, 1}},
                         {{4, 1}, {0, 0}, {5, 1}, {0, 0}, {4, 1}},
                         {{7, 1}, {2, 0}, {8, 0}, {2, 2}, {7, 3}},
                         {{4, 1}, {0, 0}, {5, 3}, {0, 0}, {4, 1}},
                         {{6, 1}, {0, 0}, {1, 3}, {0, 0}, {6, 1}},
                         {{3, 3}, {4, 0}, {7, 0}, {4, 0}, {3, 2}}};

// ちょっと多めに書かないとセグフォ起こす気がする
int intersectionDir[] = {1, 0, 1, 2 , 0, 0, 1, 2, 3};

/*
now_phaseについて
0なら直線検出モード
1なら右カーブ検出決め打ち移動状態
2なら左カーブ検出
3ならカーブ終了処理中直線検出以降状態（緩やかにカーブをやめて直線モードに移行）
4なら障害物検知状態
5なら信号検知状態
*/

/*
 * TODO
 * 右カーブ直後の横断歩道の認識が苦手なため、afterCurveSkipフラグによってスキップさせている
 */

class ImageConverter {
    ros::NodeHandle nh_;
    /* if zybo
    ros::Subscriber image_sub_;
    */
    // if_pc
    image_transport::ImageTransport it_;
    image_transport::Subscriber image_sub_;

    int Hue_l, Hue_h, Saturation_l, Saturation_h, Lightness_l, Lightness_h;
    geometry_msgs::Twist twist;

    ros::Publisher twist_pub;
    // cv::Mat curve_image;
    int line_lost_cnt;
    int curve_detect_cnt;
    int find_T;
    int find_X;

    bool find_left_line;

    std::string now_phase;

    // 発見したオブジェクト（交差点、障害物）のリスト
    std::list <OBJECT> objects;


    // 次のtileを保存
    int next_tile_x;
    int next_tile_y;
    int now_dir;

    // 検出された直線のx座標
    int detected_line_x;

    ros::Time phaseStartTime;
    ros::Time tileUpdatedTime;
    ros::Time line_lost_time;

    // change phaseで初期化
    // bottomにオブジェクトが到達したかどうか
    bool reachBottomRightLaneRightT;
    bool reachBottomRightLaneLeftT;
    bool reachBottomLeftLaneLeftT;
    bool reachBottomLeftLaneStraightEnd;

    // 交差点の挙動決定のための配列の位置
    int nowIntersectionCount;

    // BIRDS_EYE_LENGTHの3/4に右のT字路が到達したかどうか
    bool intersectionCurveStartFlagRightLaneRightT;

    // カーブの次が横断歩道の場合、カーブ終了後横断歩道を認識するまで少しストップ
    bool curveAfterCrosswalk;

    bool findLeftLaneLeftT;

    int mostUnderLeftLaneLeftT;


public:
    // コンストラクタ
    ImageConverter()
    // if pc
            : it_(nh_) {
        Hue_l = HUE_L;
        Hue_h = HUE_H;
        Saturation_l = SATURATION_L;
        Saturation_h = SATURATION_H;
        Lightness_l = LIGHTNESS_L;
        Lightness_h = LIGHTNESS_H;
        line_lost_cnt = 0;
        next_tile_x = NEXT_X;
        next_tile_y = NEXT_Y;
        now_dir = START_DIR;

        find_left_line = false;


        detected_line_x = 0;

        // start時間を初期化
        phaseStartTime = ros::Time::now();
        line_lost_time = ros::Time::now();
        tileUpdatedTime = ros::Time::now();

        now_phase = "straight";


        reachBottomRightLaneRightT = false;
        reachBottomRightLaneLeftT = false;
        reachBottomLeftLaneLeftT = false;
        reachBottomLeftLaneStraightEnd = false;
        findLeftLaneLeftT = false;
        mostUnderLeftLaneLeftT = 0;
        nowIntersectionCount = 0;
        intersectionCurveStartFlagRightLaneRightT = false;
        curveAfterCrosswalk = false;


        // カラー画像をサブスクライブ
        /* if_zybo
          image_sub_ = nh_.subscribe("/image_array", 1,
          &ImageConverter::imageCb, this);
        */
        image_sub_ = it_.subscribe("/camera/rgb/image_raw", 1,
                                   &ImageConverter::imageCb, this);

        //  処理した挙動をパブリッシュ
        //twist_pub = nh_.advertise<geometry_msgs::Twist>("/cmd_vel", 1000);
        twist_pub = nh_.advertise<geometry_msgs::Twist>("/cmd_vel", 1000);
        // 0.1秒ごとに制御を呼び出す
        //timer = nh.createTimer(ros::Duration(0.1), &ImageConverter::timerCallback, this);

        //image_pub_ = it_.advertise("/image_topic", 1);

        // twist初期化
        //geometry_msgs::Twist twist;
        twist.linear.x = 0.0;
        twist.linear.y = 0.0;
        twist.linear.z = 0.0;
        twist.angular.x = 0.0;
        twist.angular.y = 0.0;
        twist.angular.z = 0.0;
        twist_pub.publish(twist);
    }

    // デストラクタ
    ~ImageConverter() {
        // 全てのウインドウは破壊
        cv::destroyAllWindows();
    }


    // コールバック関数
    // if zybo
    // void imageCb(const std_msgs::UInt8MultiArray& msg)
    void imageCb(const sensor_msgs::ImageConstPtr &msg) {
        /* if_zybo
        cv::Mat base_image(CAMERA_HEIGHT, CAMERA_WIDTH, CV_8UC2);
        cv::Mat dstimg(CAMERA_HEIGHT, CAMERA_WIDTH, CV_8UC2);
        memcpy(base_image.data, &msg.data[0], CAMERA_WIDTH * CAMERA_HEIGHT * 2);
        cv::cvtColor(base_image, dstimg, cv::COLOR_YUV2RGB_YUYV);
        */


        // if_pc
        cv_bridge::CvImagePtr cv_ptr;
        try {
            // ROSからOpenCVの形式にtoCvCopy()で変換。cv_ptr->imageがcv::Matフォーマット。
            cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
        }
        catch (cv_bridge::Exception &e) {
            ROS_ERROR("cv_bridge exception: %s", e.what());
            return;
        }
        cv::Mat base_image = cv_ptr->image;
        ////////


        cv::Mat hsv_image, color_mask, gray_image, birds_eye;

        // 俯瞰画像

        birds_eye = birdsEye(base_image);

        cv::Mat aroundImg, aroundWhiteBinary;
        aroundImg = birdsEyeAround(base_image);
        aroundWhiteBinary = whiteBinary(aroundImg);
        cv::Mat road_white_binary(aroundWhiteBinary, cv::Rect(BIRDSEYE_LENGTH, 0, BIRDSEYE_LENGTH, BIRDSEYE_LENGTH));

        std::vector <cv::Vec4i> around_lines = getHoughLinesP(aroundWhiteBinary, 0, 10, 5);

        cv::Mat left_roi(aroundWhiteBinary, cv::Rect(BIRDSEYE_LENGTH, 0, BIRDSEYE_LENGTH / 2, BIRDSEYE_LENGTH));

        // 俯瞰画像のROIを縦中央で分割
        //cv::Mat left_roi(road_white_binary, cv::Rect(0, 0, BIRDSEYE_LENGTH / 2, BIRDSEYE_LENGTH));
        cv::Mat right_roi(road_white_binary, cv::Rect(BIRDSEYE_LENGTH / 2, 0, BIRDSEYE_LENGTH / 2, BIRDSEYE_LENGTH));

        std::vector <cv::Vec4i> left_lines = getHoughLinesP(left_roi, 20, 40, 5);

        // 交差点等のTを発見
        bool nowFingRightLaneRightT = intersectionDetection(around_lines, aroundWhiteBinary);


        // 左車線について
        // 角度平均をとり、全体の角度が垂直になるようにする
        // 最も左車線を検出し、いい感じになるよう調整する
        // 車線が見つからない場合、find_left_lineがfalseになる
        int average_cnt = 0;
        float degree_average_sum = 0;
        float most_left_middle_x = BIRDSEYE_LENGTH * 0.5;
        float robot_vel = 0;
        float robot_rot = 0;
        find_left_line = false;
        int mostDistantY = BIRDSEYE_LENGTH;
        int mostDistantX = 0;

        // 垂直に近い点のみ線を引く
        for (size_t i = 0; i < left_lines.size(); i++) {
            STRAIGHT left_line = toStraightStruct(left_lines[i]);
            if (left_line.degree < 20 && left_line.degree > -20) {

                if (left_lines[i][1] < mostDistantY) {
                    mostDistantX = left_lines[i][0];
                    mostDistantY = left_lines[i][1];
                }
                if (left_lines[i][3] < mostDistantY) {
                    mostDistantX = left_lines[i][2];
                    mostDistantY = left_lines[i][3];
                }

                //赤線を引く
                cv::line(left_roi, cv::Point(left_lines[i][0], left_lines[i][1]),
                         cv::Point(left_lines[i][2], left_lines[i][3]), cv::Scalar(0, 0, 255), 3, 8);
                degree_average_sum += left_line.degree;
                if (most_left_middle_x > left_line.middle.x) {
                    most_left_middle_x = left_line.middle.x;
                    detected_line_x = left_line.middle.x;
                }
                find_left_line = true;
                average_cnt++;
            }
        }


        // 左車線を検出できた場合
        if (find_left_line) {
            addMostDistantObject("left_lane_end", mostDistantX, mostDistantY); // 左車線の最も遠い点を直線の終点として保持しておく
            line_lost_time = ros::Time::now();
            float degree_average = degree_average_sum / average_cnt;

            // 中点が右過ぎたら左に、左過ぎたら右に曲がる
            if (detected_line_x > BIRDSEYE_LENGTH * STRAIGHT_TOO_RIGHT_LANE) {
                robot_vel = 1;
                robot_rot = -1;
            } else if (detected_line_x < BIRDSEYE_LENGTH * STRAIGHT_TOO_LEFT_LANE) {
                robot_vel = 1;
                robot_rot = 1;
            } else if (degree_average < -10 || degree_average > 10) {
                // 角度平均が-5以上なら左に曲がる、5以上なら右に曲がる
                robot_rot = degree_average * -0.2;
            } else {
                robot_vel = 3;
            }
            line_lost_cnt = 0;
        } else {
            // 車線が見つからなかった場合、LeftRoadLeftTで最下のものを基準に
            line_lost_cnt += 1;
            updateLeftLine(road_white_binary);
            robot_vel = 0;
            robot_rot = (BIRDSEYE_LENGTH * RUN_LINE - detected_line_x) / 40;
        }

        // 次のタイルの左車線が不安定な場合、LeftLaneのLeftTを優先してdetected_line_xにする
        if (line_lost_cnt > 5) {
            if (map_data[next_tile_y][next_tile_x][0] == 3 &&
                (map_data[next_tile_y][next_tile_x][1] - now_dir + 4) % 4 == 3) {
                if (findLeftLaneLeftT) {
                    detected_line_x = mostUnderLeftLaneLeftT;
                    line_lost_time = ros::Time::now();
                    // 中点が右過ぎたら左に、左過ぎたら右に曲がる
                    if (detected_line_x > BIRDSEYE_LENGTH * STRAIGHT_TOO_RIGHT_LANE) {
                        robot_vel = 1;
                        robot_rot = -1;
                    } else if (detected_line_x < BIRDSEYE_LENGTH * STRAIGHT_TOO_LEFT_LANE) {
                        robot_vel = 1;
                        robot_rot = 1;
                    } else {
                        robot_vel = 1;
                    }
                }
            }
        }

        // ---------------controller----------------
        updateObject();

        if (now_phase == "straight") {
            ros::Time now = ros::Time::now();
            if (now - line_lost_time > ros::Duration(5.0)) {
                changePhase("search_line");
            } else {
                findRedObs(birds_eye);
                searchTile();
                lineTrace(robot_vel, robot_rot);
            }
        } else if (now_phase == "trace_right_curve") {
            rightCurveTrace(road_white_binary);
        } else if (now_phase == "search_line") {
            searchLine();
        } else if (now_phase == "search_right_lane_right_T") {
            searchRightLaneRightT(nowFingRightLaneRightT);
        } else if (now_phase == "turn_left") {
            leftTurn();
        } else if (now_phase == "turn_right") {
            determinationRightTurn();
        } else if (now_phase == "find_obs") {
            obstacleAvoidance(right_roi);
        }
        /*
        else if ("detect_right_line")
        {

        } else if ("start_calibration")
        {
        }
         */

        // ---------------controller end----------------

        // 以下デバッグ用
        // 画像サイズを縦横半分に変更

        updateLeftLine(road_white_binary);


        ////////////

        for (OBJECT object : objects) {
            if (object.objType == "right_lane_right_T") {
                cv::ellipse(road_white_binary, cv::Point(object.beforeX + BIRDSEYE_LENGTH / 2, object.beforeY),
                            cv::Size(10, 10), 0, 0, 360, cv::Scalar(0, 200, 0), -1, 8);
            } else if (object.objType == "left_lane_left_T" || object.objType == "right_lane_left_T") {
                cv::ellipse(road_white_binary, cv::Point(object.beforeX + BIRDSEYE_LENGTH / 2, object.beforeY),
                            cv::Size(10, 10), 0, 0, 360, cv::Scalar(0, 0, 200), -1, 8);
            }
        }



        cv::Mat cv_half_image, birds_eye_x4, white_binary_x4, left_roi_x4, right_roi_x4, aroundImg_x4, aroundWhiteBinary_x4, red_image_x4;
        cv::resize(base_image, cv_half_image, cv::Size(), 0.25, 0.25);
        cv::resize(birds_eye, birds_eye_x4, cv::Size(), 4, 4);
        cv::resize(road_white_binary, white_binary_x4, cv::Size(), 4, 4);
        cv::resize(left_roi, left_roi_x4, cv::Size(), 4, 4);
        cv::resize(right_roi, right_roi_x4, cv::Size(), 4, 4);
        cv::resize(aroundImg, aroundImg_x4, cv::Size(), 2, 2);
        cv::resize(aroundWhiteBinary, aroundWhiteBinary_x4, cv::Size(), 2, 2);
        //cv::resize(red_image, red_image_x4, cv::Size(), 2, 2);

        // ウインドウ表示
        cv::imshow("Original Image", cv_half_image);
        cv::imshow("WHITE BINARY", white_binary_x4);
        // cv::imshow("ROI", birds_eye_x4);
        cv::imshow("LEFT ROI", left_roi_x4);
        //cv::imshow("RIGHT ROI",  right_roi_x4);
        // cv::imshow("road hough", road_hough);
        cv::imshow("center line", aroundImg_x4);
        cv::imshow("aroundWhite", aroundWhiteBinary_x4);
        //cv::imshow("Red Image", red_image_x4);

        cv::waitKey(3);

        //エッジ画像をパブリッシュ。OpenCVからROS形式にtoImageMsg()で変換。
        //image_pub_.publish(cv_ptr3->toImageMsg());
    }

////////////////関数//////////////////
    // phaseの変更ともろもろの値の初期化
    void changePhase(std::string next_phase) {
        std::cout << "change phase!" << next_phase << std::endl;
        // 前のphaseの結果によって変更される値を処理する
        now_phase = next_phase;
        phaseStartTime = ros::Time::now();
        resetFlag();
    }

    void resetFlag() {
        objects.clear();
        curve_detect_cnt = 0;
        reachBottomLeftLaneLeftT = false;
        findLeftLaneLeftT = false;
        reachBottomRightLaneLeftT = false;
        reachBottomRightLaneRightT = false;
        intersectionCurveStartFlagRightLaneRightT = false;
        reachBottomLeftLaneStraightEnd = false;
        line_lost_time = ros::Time::now();
    }

    // タイルは直進中(now_phase = "straight")のときみ検索する
    // タイルを見つけた時の処理は、dir（進行方角）の変更、次タイルの決定、now_phaseの変更、reachBottomObject類の初期化, 交差点ならnowIntersectionCountを進める
    // T字路などが画面下部に到達したことを利用するならばフラグをリセット
    // 長いけど何でもするから許して（何でもするとは言っていない）
    void searchTile() {
        ros::Time now = ros::Time::now();

        // タイルの種類 1~8がそれぞれFPTのroad meshに対応
        int tileType = map_data[next_tile_y][next_tile_x][0];

        // タイルの回転 1が画像通りで0~3で表している
        int tileRot = map_data[next_tile_y][next_tile_x][1];


        // タイルと入射角の差　どの方角からタイルに侵入するかを判別
        int differenceDirection = (tileRot - now_dir + 4) % 4;
        // 交差点で次にどの方角へ向かうかが決められているので、それと現在の方角の差をとるために使う
        int nextDirection = (intersectionDir[nowIntersectionCount] - now_dir + 4) % 4;

        if (tileType == 3 && differenceDirection== 2) {
            // nextTileを検索
            // カーブを右に曲がるならfind_curveを探索
            if (now - line_lost_time > ros::Duration(RIGHT_CURVE_START_LOST_LINE_TIME)) {
                curveAfterCrosswalk = true;
                now_dir = (now_dir + 1) % 4;
                changePhase("trace_right_curve");
                setNextTile();
            }
        } else if (tileType == 3 && differenceDirection == 3) {
            // 左カーブ
            if (reachBottomLeftLaneLeftT) {
                now_dir = (now_dir + 3) % 4;
                changePhase("turn_left");
                setNextTile();
            }
        } else if (tileType == 6) {
            // 横断歩道
            if (reachBottomRightLaneRightT) {
                std::cout << "横断歩道発見" << std::endl;
                resetFlag();
                setNextTile();
            }
        } else if (tileType == 7) { // T字路
            if(differenceDirection == 3) {
                // T字路に左から入る
                if (nextDirection == 0) { // 直進
                    if (reachBottomRightLaneRightT) {
                        nowIntersectionCount++;
                        setNextTile();
                    }
                } else { // 右に曲がる
                    if (intersectionCurveStartFlagRightLaneRightT) {
                        nowIntersectionCount++;
                        now_dir = (now_dir + 1) % 4;
                        changePhase("turn_right");
                        setNextTile();
                    }
                }
            } else if(differenceDirection == 0) {
                // T字路の下から突き当りに向かって入った場合
                if (nextDirection == 1) { // 右に曲がる
                    if (reachBottomLeftLaneLeftT) {
                        nowIntersectionCount++;
                        now_dir = (now_dir + 1) % 4;
                        changePhase("turn_right");
                        setNextTile();
                    }
                } else { // 左に曲がる
                    if (reachBottomLeftLaneLeftT) {
                        nowIntersectionCount++;
                        now_dir = (now_dir + 3) % 4;
                        changePhase("turn_left");
                        setNextTile();
                    }
                }
            } else { // T字路に右から入った場合
                if (nextDirection == 0){ // 直進 左車線が消えるため、特殊な動作をさせる

                }
                if (nextDirection == 3) { // 左に曲がる
                    if (reachBottomLeftLaneLeftT) {
                        nowIntersectionCount++;
                        now_dir = (now_dir + 3) % 4;
                        changePhase("turn_left");
                        setNextTile();
                    }
                }

            }

        } else if (tileType == 8) {
            if (nextDirection == 1) {
                if (intersectionCurveStartFlagRightLaneRightT) {
                    nowIntersectionCount++;
                    std::cout << "十字路を右に曲がる" << std::endl;
                    now_dir = (now_dir + 1) % 4;
                    changePhase("turn_right");
                    setNextTile();
                }
            } else if (nextDirection == 3) {
                if (reachBottomLeftLaneLeftT) {
                    nowIntersectionCount++;
                    now_dir = (now_dir + 3) % 4;
                    changePhase("turn_left");
                    setNextTile();
                }
            }
        }
    }


    // タイルが見つかったときに呼び出される
    // 今next_tileとなっているものを現在位置とし、今の方向と現在位置から次のタイル目標を決定する
    // road4は特徴のない直線のため無視する
    void setNextTile() {
        int next_x = next_tile_x;
        int next_y = next_tile_y;

        bool find_tile = false;

        // road4をスキップするために繰り返す
        while (!find_tile) {

            // 今の進行方向によって次のタイルを検索
            // 0が南で右回り, 原点は左上
            switch (now_dir) {
                case 0:
                    next_x = next_x;
                    next_y = next_y + 1;
                    break;
                case 1:
                    next_x = next_x - 1;
                    next_y = next_y;
                    break;
                case 2:
                    next_x = next_x;
                    next_y = next_y - 1;
                    break;
                default:
                    next_x = next_x + 1;
                    next_y = next_y;
                    break;
            }

            // road4(ただの直線)でないかチェック
            // 今だけカーブ検索中！！！！
            int nextTile = map_data[next_y][next_x][0];

            // カーブの後はcurveAfterCrosswalkがtrueになっているので、直後のnextTileが横断歩道の時のみtrueのまま
            // 別の場合はcurveAfterCrosswalkをfalseにする
            if (nextTile == 6) {
                find_tile = true;
            } else if (nextTile == 3 || nextTile == 7 || nextTile == 8) {
                find_tile = true;
                curveAfterCrosswalk = false;
            } else {
                curveAfterCrosswalk = false;
            }

        }

        // next_tileの更新
        next_tile_x = next_x;
        next_tile_y = next_y;
        std::cout << "next tile " << next_x << " " << next_y << "   type=" << map_data[next_y][next_x][0] << std::endl;
        std::cout << "now dir  " << now_dir << std::endl;

    }
    /////////実際に動かす関数//////////////////

    // 決め打ちで左カーブ
    // 入射時の速度でカーブ時間を変更
    void leftTurn() {
        twist.linear.x = LEFT_CURVE_VEL;
        twist.angular.z = LEFT_CURVE_ROT;
        ros::Time now = ros::Time::now();
        if (now - phaseStartTime > ros::Duration(LEFT_CURVE_END_TIME + LEFT_CURVE_END_MARGIN_TIME)) {
            changePhase("search_line");
        } else if (now - phaseStartTime > ros::Duration(LEFT_CURVE_END_TIME)) {
            twist.angular.z = LEFT_CURVE_AFTER_ROT;
        }
        twist_pub.publish(twist);
    }

    // 決め打ちで右カーブ
    void determinationRightTurn() {
        twist.linear.x = RIGHT_CURVE_VEL;
        twist.angular.z = RIGHT_CURVE_ROT;
        ros::Time now = ros::Time::now();
        if (now - phaseStartTime > ros::Duration(RIGHT_CURVE_END_TIME) && find_left_line) {
            changePhase("search_line");
        } else if (now - phaseStartTime > ros::Duration(RIGHT_CURVE_END_TIME + RIGHT_CURVE_END_MARGIN_TIME)) {
            changePhase("search_line");
        }
        twist_pub.publish(twist);
    }

    // 障害物検知
    // 決め打ちで右にカーブし、決め打ちで左に戻る
    void  obstacleAvoidance(cv::Mat rightROI) {
        ros::Time now = ros::Time::now();
        //　右車線に向けて回転
        if (now - phaseStartTime <  ros::Duration(AVOID_ROT_TIME)) {
            twist.linear.x = AVOID_OBSTACLE_VEL;
            twist.angular.z = AVOID_OBSTACLE_ROT;
        } else if(now - phaseStartTime <  ros::Duration(AVOID_ROT_TIME + AVOID_ROT_STRAIGHT))
        { // 右車線に向けて直進
            twist.linear.x = AVOID_OBSTACLE_VEL;
            twist.angular.z = 0;
        } else if(now - phaseStartTime <  ros::Duration(AVOID_ROT_TIME * 2 + AVOID_ROT_STRAIGHT))
        { // 右車線に対して水平になるように回転
            twist.linear.x = AVOID_OBSTACLE_VEL;
            twist.angular.z = -1 * AVOID_OBSTACLE_ROT;
        } else if(now - phaseStartTime <  ros::Duration(AVOID_ROT_TIME * 2 + AVOID_ROT_STRAIGHT + AVOID_BEFORE_STRAIGHT_MARGIN_TIME))
        { // 直進向く寸前に反動を消す
            twist.linear.x = AVOID_OBSTACLE_VEL;
            twist.angular.z = AVOID_OBSTACLE_ROT / 5;
        }else if(now - phaseStartTime <  ros::Duration(AVOID_ROT_TIME * 2 + AVOID_ROT_STRAIGHT + AVOID_STRAIGHT_TIME)) { // 右車線を反転させてライントレースすることで、左車線と同様のアルゴリズムで走らせる(注// アングルも逆)
            cv::Mat flipImg;
            cv::flip(rightROI, flipImg, 1);
            std::vector <cv::Vec4i> rightLines = getHoughLinesP(flipImg, 20, 40, 5);
            float degreeAverageSum = 0;
            float most_left_middle_x = BIRDSEYE_LENGTH * 0.5;
            int average_cnt = 0;
            bool findRightLane = false;

            for (size_t i = 0; i < rightLines.size(); i++) {
                STRAIGHT rightLine = toStraightStruct(rightLines[i]);
                if (rightLine.degree < 20 && rightLine.degree > -20) {
                    degreeAverageSum += rightLine.degree;
                    if (most_left_middle_x > rightLine.middle.x) {
                        most_left_middle_x = rightLine.middle.x;
                        detected_line_x = rightLine.middle.x;
                    }
                    findRightLane= true;
                    average_cnt++;
                }
            }
            // 左車線を検出できた場合
            if (findRightLane) {
                line_lost_time = ros::Time::now();
                float degree_average = degreeAverageSum / average_cnt;

                // 中点が右過ぎたら左に、左過ぎたら右に曲がる
                if (detected_line_x > BIRDSEYE_LENGTH * STRAIGHT_TOO_RIGHT_LANE) {
                    twist.angular.z = 0.1;
                } else if (detected_line_x < BIRDSEYE_LENGTH * STRAIGHT_TOO_LEFT_LANE) {
                    twist.angular.z = -0.1;
                } else if (degree_average < -10) {
                    twist.angular.z = -0.1;
                } else if (degree_average > 10) {
                    twist.angular.z = 0.1;
                }
            } else {
                std::cout << "lost line" << std::endl;
                twist.angular.z = 0;
            }
            /*else {
                // 車線が見つからなかった場合、LeftRoadLeftTで最下のものを基準に
                std::cout << "lost line" << std::endl;
                updateLeftLine(flipImg);
                twist.angular.z = (BIRDSEYE_LENGTH * RUN_LINE - detected_line_x) / -400;
            }*/
            twist.linear.x = 0.2;
            std::cout << "left line trace  " << twist.angular.z << std::endl;
        } else if(now - phaseStartTime <  ros::Duration(AVOID_ROT_TIME * 3 + AVOID_ROT_STRAIGHT + AVOID_STRAIGHT_TIME))
        { // 左車線に向けて回転
            twist.linear.x = AVOID_OBSTACLE_VEL;
            twist.angular.z = -1 * AVOID_OBSTACLE_ROT;
        } else if(now - phaseStartTime <  ros::Duration(AVOID_ROT_TIME * 3 + AVOID_ROT_STRAIGHT * 2 + AVOID_STRAIGHT_TIME))
        { // 左車線に向けて直進
            twist.linear.x = AVOID_OBSTACLE_VEL;
            twist.angular.z = 0;
        } else if(now - phaseStartTime <  ros::Duration(AVOID_ROT_TIME * 4 + AVOID_ROT_STRAIGHT * 2 + AVOID_STRAIGHT_TIME))
        { //左車線と水平になるように回転
            twist.linear.x = AVOID_OBSTACLE_VEL;
            twist.angular.z = AVOID_OBSTACLE_ROT;
        } else {
            changePhase("straight");
        }
        twist_pub.publish(twist);
    }

    // カーブを曲がるときにラインを追跡して挙動決定
    // 交差点で曲がる時はまた別
    void rightCurveTrace(cv::Mat road_binary) {

        int before_line_x = detected_line_x;

        // detected_line_xをラインの左下から更新する
        updateLeftLine(road_binary);

        // ロボットの速度決定
        twist.linear.x = 0.2;
        twist.angular.z = (BIRDSEYE_LENGTH * RUN_LINE - detected_line_x) / 40;
        twist_pub.publish(twist);

        // 終了処理
        ros::Time now = ros::Time::now();
        if (now - phaseStartTime > ros::Duration(RIGHT_CURVE_END_TIME) && find_left_line) {
            if (curveAfterCrosswalk) {
                curveAfterCrosswalk = false;
                changePhase("search_right_lane_right_T");
            } else {
                changePhase("search_line");
            }
        } else if (now - phaseStartTime > ros::Duration(RIGHT_CURVE_END_TIME + RIGHT_CURVE_END_MARGIN_TIME)) {
            if (curveAfterCrosswalk) {
                curveAfterCrosswalk = false;
                changePhase("search_right_lane_right_T");
            } else {
                changePhase("search_line");
            }
        }
    }


    // もし次のタイルがTもしくは左カーブの場合、mostUnderLeftLaneLeftTから更新する
    // 白に二値化された画像から一番左下のラインを読み取ってdetected_line_xを更新する
    void updateLeftLine(cv::Mat road_binary) {
        cv::Mat road_hough;
        cv::Canny(road_binary, road_hough, 50, 200, 3);

        // 白を見つけたらbaseを更新して、10ピクセル以内にまた白があればそれを仮の白線とする。
        int temp_detected_line = detected_line_x;


        // 複数あった場合、直前のライントレースの結果との差を利用する
        int temp_dif = BIRDSEYE_LENGTH / 2;
        // uchar *road_hough_bottom = road_hough.ptr<uchar>(BIRDSEYE_LENGTH - 1);
        for (int i = 0; i < detected_line_x + BIRDSEYE_LENGTH / 10; i++) {
            int p = road_hough.at<uchar>(BIRDSEYE_LENGTH * HEIGHT_L - 1, i);
            if (p) {
                for (int j = i + 1; j < i + BIRDSEYE_LENGTH / 10; j++) {

                    int q = road_hough.at<uchar>(BIRDSEYE_LENGTH * HEIGHT_L - 1, j);
                    std::cout << "updateleft i " << i << "  q " << q << std::endl;
                    if (q) {
                        int this_dif = (i + j) / 2 - detected_line_x;
                        if (this_dif < temp_dif) {
                            temp_dif = this_dif;
                            temp_detected_line = (i + j) / 2;
                        }
                    }
                }
            }
        }
        std::cout << "temp_detected_line" << temp_detected_line << std::endl;
        detected_line_x = temp_detected_line;
    }


    // 確率ハフ変換によってラインを得る
    std::vector <cv::Vec4i> getHoughLinesP(cv::Mat image, int threshold, double minLineLength, double maxLineGap) {
        // 左側をハフ変換
        cv::Mat temp_dst, temp_color_dst;
        cv::Canny(image, temp_dst, 50, 200, 3);
        // cv::cvtColor( temp_dst, temp_color_dst, CV_GRAY2BGR );
        std::vector <cv::Vec4i> lines;
        cv::HoughLinesP(temp_dst, lines, 1, CV_PI / 180, threshold, minLineLength, maxLineGap);
        return lines;
    }

    // ライン検出の結果によって左右に操作
    // ラインがあればtrue
    // intは+1で左, 0で直進, -1で右
    // ラインが見つからなければ左に回転
    void lineTrace(int vel, int dir) {

        twist.linear.x += 0.01 * vel;
        twist.angular.z = 0.1 * dir;

        if (twist.linear.x > BURGER_MAX_LIN_VEL) twist.linear.x = BURGER_MAX_LIN_VEL;
        if (twist.linear.x < 0) twist.linear.x = 0;
        if (twist.angular.z > BURGER_MAX_ANG_VEL) twist.angular.z = BURGER_MAX_ANG_VEL;
        if ((twist.angular.z < 0 - BURGER_MAX_ANG_VEL)) twist.angular.z = 0 - BURGER_MAX_ANG_VEL;
        twist_pub.publish(twist);
    }

    // ラインが見つからないときに首を振ることで直線を探す
    void searchLine() {
        ros::Time now = ros::Time::now();
        if (find_left_line) {
            changePhase("straight");
            twist.angular.z = -1 * twist.angular.z;
            twist_pub.publish(twist);
            return;
        }
        std::cout << "line search" << std::endl;


        // 三秒ごとに首を振る向きを変える
        if (now - phaseStartTime < ros::Duration(3.0)) {
            twist.linear.x = 0;
            twist.angular.z = -0.1;
        } else if (now - phaseStartTime < ros::Duration(6.0)) {
            twist.angular.z = 0.1;
        } else if (now - phaseStartTime < ros::Duration(7.0)) {
            twist.angular.z = 0.3;
        } else if (now - phaseStartTime < ros::Duration(9.0)) {
            twist.angular.z = 0.3;
        } else {
            phaseStartTime = ros::Time::now();
            std::cout << "one more search" << std::endl;
        }

        twist_pub.publish(twist);
    }

    void searchRightLaneRightT(bool nowFindRightLaneRightT) {
        ros::Time now = ros::Time::now();
        if (nowFindRightLaneRightT) {
            changePhase("straight");
            twist.angular.z = -1 * twist.angular.z;
            twist_pub.publish(twist);
            return;
        }
        std::cout << "now search right_lane_right_T " << std::endl;

        // 三秒ごとに首を振る向きを変える
        if (now - phaseStartTime < ros::Duration(3.0)) {
            twist.linear.x = 0;
            twist.angular.z = -0.1;
        } else if (now - phaseStartTime < ros::Duration(6.0)) {
            twist.angular.z = 0.1;
        } else if (now - phaseStartTime < ros::Duration(7.0)) {
            twist.angular.z = 0.3;
        } else if (now - phaseStartTime < ros::Duration(9.0)) {
            twist.angular.z = 0.3;
        } else {
            phaseStartTime = ros::Time::now();
            std::cout << "one more search" << std::endl;
        }
        twist_pub.publish(twist);
    }



    // imageを渡して俯瞰画像を得る
    cv::Mat birdsEye(cv::Mat image) {
        int width = image.size().width;
        int height = image.size().height;
        // 奥行の広さ（小さいほど狭い）
        float width_ratio = WIDTH_RATIO;
        // 上部
        float height_h = HEIGHT_H;
        // 下部
        float height_l = HEIGHT_L;
        // 画素値
        int result_size = BIRDSEYE_LENGTH;
        cv::Mat map_matrix, dst_image;
        cv::Point2f src_pnt[4], dst_pnt[4];

        src_pnt[0] = cv::Point(width * (0.5 - width_ratio), height * height_h);
        src_pnt[1] = cv::Point(0, height * height_l);
        src_pnt[2] = cv::Point(width * (0.5 + width_ratio), height * height_h);
        src_pnt[3] = cv::Point(width, height * height_l);

        dst_pnt[0] = cv::Point(0, 0);
        dst_pnt[1] = cv::Point(0, result_size);
        dst_pnt[2] = cv::Point(result_size, 0);
        dst_pnt[3] = cv::Point(result_size, result_size);

        map_matrix = cv::getPerspectiveTransform(src_pnt, dst_pnt);
        cv::warpPerspective(image, dst_image, map_matrix, cv::Size(result_size, result_size));
        return dst_image;
    }

    // imageを渡して中央線の画像を得る
    //　精度が甘くても、一番下から表示できるように設定
    cv::Mat birdsEyeAround(cv::Mat image) {
        int width = image.size().width;
        int height = image.size().height;
        // 奥行の広さ（小さいほど狭い）
        float width_ratio = WIDTH_RATIO;
        // 上部
        float height_h = HEIGHT_H;
        // 下部
        float height_l = HEIGHT_L;
        // 画素値
        int result_size = BIRDSEYE_LENGTH;
        cv::Mat map_matrix, dstImageCenter;
        cv::Point2f src_pnt[4], dst_pnt[4];

        src_pnt[0] = cv::Point(width * (0.5 - width_ratio), height * height_h);
        src_pnt[1] = cv::Point(width * (0 - (1 - height_l) * (0.5 - width_ratio) / (height_l - height_h)), height);
        src_pnt[2] = cv::Point(width * (0.5 + width_ratio), height * height_h);
        src_pnt[3] = cv::Point(width * (1 + (1 - height_l) * (0.5 - width_ratio) / (height_l - height_h)), height);

        dst_pnt[0] = cv::Point(result_size, 0);
        dst_pnt[1] = cv::Point(result_size, result_size);
        dst_pnt[2] = cv::Point(result_size * 2, 0);
        dst_pnt[3] = cv::Point(result_size * 2, result_size);

        map_matrix = cv::getPerspectiveTransform(src_pnt, dst_pnt);
        cv::warpPerspective(image, dstImageCenter, map_matrix, cv::Size(result_size * 3, result_size), cv::INTER_LINEAR,
                            cv::BORDER_CONSTANT, cv::Scalar::all(100));
        return dstImageCenter;
    }

    // 画像の中から一番下の障害物を検知
    // wideViewから検索
    // まず、車体正面のBIRDSLENGTH四方を取り出し、赤色っぽいものの二値化を行う
    // y軸方向で切り出し、一番yが大きいものをy座標としてobjectsに追加or更新
    // 全体の赤色値を信頼度とし、一定値以上あれば障害物として認知
    // objectのy座標が一定以下になれば回避行動フェイズにチェンジ
    /*
    void detectObstacle(){

    }
     */


// 二点をSTRAIGHT構造体で返す
    STRAIGHT toStraightStruct(cv::Vec4i line) {
        STRAIGHT result;

        //中点
        result.middle = cv::Point((line[0] + line[2]) / 2, (line[1] + line[3]) / 2);
        // 距離
        result.length = (line[0] - line[2]) * (line[0] - line[2]) + (line[1] - line[3]) * (line[1] - line[3]);
        // 角度
        float radian = atan2(line[1] - line[3], line[0] - line[2]);
        // radianからdegree

        if (radian * 180.0 / PI >= 90) {
            result.degree = radian * 180.0 / PI - 90;
        } else {
            result.degree = radian * 180.0 / PI + 90;
        }
        return result;
    }

// 二点間の傾きを求め、長さをかけて重さとする
// x1 y1, x2 y2
    float lineWeight(cv::Vec4i line) {
        // 距離
        float distance = (line[0] - line[2]) * (line[0] - line[2]) + (line[1] - line[3]) * (line[1] - line[3]);

        // 角度
        float radian = atan2(line[1] - line[3], line[0] - line[2]);

        // radianからdegree
        float degree = radian * 180.0 / PI;

        return degree;
    }


// 白色検出（返り値はRGB）
    cv::Mat whiteBinary(cv::Mat image) {
        cv::Mat color_mask, result_image, hsv_image;
        cv::cvtColor(image, hsv_image, CV_BGR2HSV);
        cv::inRange(hsv_image, cv::Scalar(Hue_l, Saturation_l, Lightness_l, 0),
                    cv::Scalar(Hue_h, Saturation_h, Lightness_h, 0), color_mask);
        cv::bitwise_and(image, image, result_image, color_mask);

        return result_image;
    }


    // 水平のラインと縦のラインが近しいか計測
    // 片方の点がもう一つの線のx,yで形作られる◇の中に存在するかどうかで判別
    // 1なら右、-1なら左を示す
    int crossCheck(cv::Vec4i horiLine, cv::Vec4i verLine) {
        int dir = 0;
        if ((horiLine[0] > verLine[0] - 5) && (horiLine[0] < verLine[2] + 5)) {
            if ((horiLine[1] > verLine[1] - 5) && (horiLine[1] < verLine[3] + 5)) {
                dir = 1;
            } else if ((horiLine[1] > verLine[3] - 5) && (horiLine[1] < verLine[1] + 5)) {
                dir = 1;
            }
        } else if ((horiLine[2] > verLine[0] - 5) && (horiLine[2] < verLine[2] + 5)) {
            if ((horiLine[3] > verLine[1] - 5) && (horiLine[3] < verLine[3] + 5)) {
                dir = -1;
            } else if ((horiLine[3] > verLine[3] - 5) && (horiLine[3] < verLine[1] + 5)) {
                dir = -1;
            }
        }
        return dir;
    }


    // 画像から車線から伸びているTを検知し、オブジェクトリストに追加、更新する。
    // right_lane_right_Tを発見したかをboolで返す
    bool intersectionDetection(std::vector <cv::Vec4i> lines, cv::Mat whiteImg) {
        bool nowFindRightLaneRightT = false;
        // 右画像に対して,Tや十が存在するかサーチする
        for (int i = 0; i < lines.size(); i++) {
            STRAIGHT right_line = toStraightStruct(lines[i]);
            // 水平に近い
            if (right_line.degree > 70 || right_line.degree < -70) {
                for (int j = 0; j < lines.size(); j++) {
                    if (j == i) continue;
                    STRAIGHT check_line = toStraightStruct(lines[j]);

                    // 比較する対象を垂直に絞る
                    if (std::abs(std::abs(check_line.degree - right_line.degree) - 90) < 10) {
                        int dir = (crossCheck(lines[i], lines[j]));
                        // pointがpoint二点の間にあるかどうか調べる関数
                        if (dir == 1 & lines[i][1] > 30 && lines[i][2] > BIRDSEYE_LENGTH * 1.5 && lines[i][2] < BIRDSEYE_LENGTH * 2.2) { // 右に伸びていて、かつある程度下にある場合
                            addObject("right_lane_right_T", lines[i][0], lines[i][1]);
                            nowFindRightLaneRightT = true;
                            cv::line(whiteImg, cv::Point(lines[i][0], lines[i][1]),
                                     cv::Point(lines[i][2], lines[i][3]), cv::Scalar(0, 255, 0), 3, 8);
                            cv::line(whiteImg, cv::Point(lines[j][0], lines[j][1]),
                                     cv::Point(lines[j][2], lines[j][3]), cv::Scalar(0, 255, 0), 3, 8);
                        } else if (dir == -1 && lines[i][3] > 30) {
                            if (lines[i][2] > BIRDSEYE_LENGTH * 0.5 && lines[i][2] < BIRDSEYE_LENGTH * 1.5) {
                                addObject("left_lane_left_T", lines[i][2], lines[i][3]);
                            } else if (lines[i][2] > BIRDSEYE_LENGTH * 1.5 && lines[i][2] < BIRDSEYE_LENGTH * 2.5) {
                                addObject("right_lane_left_T", lines[i][2], lines[i][3]);
                            }
                            cv::line(whiteImg, cv::Point(lines[i][0], lines[i][1]),
                                     cv::Point(lines[i][2], lines[i][3]), cv::Scalar(0, 0, 255), 3, 8);
                            cv::line(whiteImg, cv::Point(lines[j][0], lines[j][1]),
                                     cv::Point(lines[j][2], lines[j][3]), cv::Scalar(0, 0, 255), 3, 8);
                        }
                    }
                }
            }
        }

        return nowFindRightLaneRightT;
    }


    // objTypeに一致するオブジェクトをすべて消去
    void deleteObject(std::string objType) {
        std::list<OBJECT>::iterator itr;
        for (itr = objects.begin(); itr != objects.end();) {
            OBJECT compare = *itr;
            if (compare.objType == objType) {
                itr = objects.erase(itr);
                continue;
            }
            itr++;
        }
    }

    // オブジェクトを最も遠いもの(Yが小さいもの)に更新
    // ひとつ見つけた時点で終了するため、この関数で追加するobjTypeはaddObjectを用いない
    void addMostDistantObject(std::string objType, int objectX, int objectY) {
        bool findObj = false;
        std::list<OBJECT>::iterator itr;
        for (itr = objects.begin(); itr != objects.end();) {
            OBJECT compare = *itr;
            if (compare.objType == objType && objectY < compare.beforeY) {
                std::cout << "update object" << std::endl;
                compare.beforeX = objectX;
                compare.beforeY = objectY;
                compare.findCnt += 1;
                compare.timeStamp = ros::Time::now();
                *itr = compare;
                findObj = true;
                break;
            }
            itr++;
        }
        if (!findObj) {
            OBJECT obj = {objType, objectX, objectY, 1, ros::Time::now()};
            objects.push_front(obj);
        }
    }

    // オブジェクトを発見した時、それが以前発見されたものと一致するかどうかを調べ、一致しなかったら追加
    // 一致する場合タイムスタンプと位置を更新し、カウントを1増やす
    void addObject(std::string objType, int objectX, int objectY) {
        bool findObj = false;
        /*
        for(OBJECT compare : objects) {
          if( compare.objType == objType && (objectX > compare.beforeX - 15) && (std::abs(objectY - compare.beforeY) < 30)) {
            compare.beforeX = objectX;
            compare.beforeY = objectY;
            compare.findCnt += 1;
            compare.timeStamp = ros::Time::now();
            findObj = true;
            break;
          }
        }
         */
        std::list<OBJECT>::iterator itr;
        for (itr = objects.begin(); itr != objects.end();) {
            OBJECT compare = *itr;
            if (compare.objType == objType && (objectX > compare.beforeX - 5) &&
                (std::abs(objectY - compare.beforeY) < 10)) {
                compare.beforeX = objectX;
                compare.beforeY = objectY;
                compare.findCnt += 1;
                compare.timeStamp = ros::Time::now();
                *itr = compare;
                findObj = true;
                break;
            }
            itr++;
        }
        if (!findObj) {
            OBJECT obj = {objType, objectX, objectY, 1, ros::Time::now()};
            objects.push_back(obj);
        }
    }

    void findRedObs(cv::Mat birds_eye){
        cv::Mat red_mask1, red_mask2, red_image, red_hsv_image;
        cv::Mat redRoi(birds_eye, cv::Rect(BIRDSEYE_LENGTH * 0.2, BIRDSEYE_LENGTH / 2, BIRDSEYE_LENGTH / 2, BIRDSEYE_LENGTH / 2));
        cv::cvtColor(redRoi, red_hsv_image, CV_BGR2HSV);
        cv::inRange(red_hsv_image, cv::Scalar(0, 127, 0, 0),
                    cv::Scalar(15, 255, 255, 0), red_mask1);
        cv::inRange(red_hsv_image, cv::Scalar(150, 127, 0, 0),
                    cv::Scalar(179, 255, 255, 0), red_mask2);
        // cv::bitwise_and(redRoi, redRoi, red_image, red_mask1 + red_mask2);

        int fractionNum = cv::countNonZero(red_mask1 + red_mask2);
        if (fractionNum > 500) {
            changePhase("find_obs");
        }
    }


    // オブジェクトが一定時間発見されていなければ破棄
    void updateObject() {
        ros::Time now = ros::Time::now();
        std::list<OBJECT>::iterator itr;
        int objCnt;
        int mostUnderLeftLaneLeftT_y = 0;
        bool nowFindLeftLaneLeftT = false;
        mostUnderLeftLaneLeftT = detected_line_x;


        for (itr = objects.begin(); itr != objects.end();) {
            OBJECT obj = *itr;


            // 交差点がBIRDSEYE_LENGTHの3/4に到達するタイミングを推定し、到達するとintersectionCurveStartFlagRightLaneRightTを立てる
            if (obj.objType == "right_lane_right_T") {
                double  reachCurveStartTime = (INTERSECTION_CURVE_START_FLAG_RATIO - ((double)obj.beforeY) / BIRDSEYE_LENGTH) * 4 * INTERSECTION_PREDICTION_TIME_RATIO * (0.2 / (twist.linear.x + 0.001));
                if (now - obj.timeStamp > ros::Duration(reachCurveStartTime)) {
                    if (obj.findCnt > 1) {
                        intersectionCurveStartFlagRightLaneRightT = true;
                    }
                }
            }


            // オブジェクトが下に到達する時刻を推定し、下に到達したと推定された場合アクションのためのフラグを立てる
            // タイルを進める、左に曲がる等をsearchTile()で行う
            double  reachBottomTime = ((1 - ((double)obj.beforeY) / BIRDSEYE_LENGTH) * 4  + INTERSECTION_PREDICTION_UNDER_MARGIN) * INTERSECTION_PREDICTION_TIME_RATIO * (0.2 / (twist.linear.x + 0.001));
            if (now - obj.timeStamp > ros::Duration(reachBottomTime)) {
                if (obj.findCnt > 1) {
                    if (obj.objType == "left_lane_left_T") {
                        reachBottomLeftLaneLeftT = true;
                    } else if (obj.objType == "right_lane_left_T") {
                        reachBottomRightLaneLeftT = true;
                    } else if (obj.objType == "right_lane_right_T") {
                        reachBottomRightLaneRightT = true;
                    } else if (obj.objType == "left_lane_end") {
                        reachBottomLeftLaneStraightEnd = true;
                    }
                }
                itr = objects.erase(itr);
                continue;
            }
            if (obj.objType == "left_lane_left_T") {
                nowFindLeftLaneLeftT = true;
                if (mostUnderLeftLaneLeftT_y > obj.beforeY) {
                    mostUnderLeftLaneLeftT_y = obj.beforeY;
                    mostUnderLeftLaneLeftT = obj.beforeX - BIRDSEYE_LENGTH; // BIRDSEYE_LENGTH分だけ右にずれているため
                }
            }
            itr++;
            objCnt++;
        }

        // メンバ変数を更新
        if (nowFindLeftLaneLeftT) {
            findLeftLaneLeftT = true;
        } else {
            findLeftLaneLeftT = false;
        }
    }
};

int main(int argc, char **argv) {
    ros::init(argc, argv, "image_converter");
    ImageConverter ic;
    ros::spin();
    return 0;
}
