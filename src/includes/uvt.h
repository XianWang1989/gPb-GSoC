#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <queue>
#include <vector>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/core.hpp>

#include <float.h>

#define DOUBLE_SIZE 1
#define SINGLE_SIZE 0

namespace cv {
void uvt(const cv::Mat & ucm_mtr,
         const cv::Mat & seeds,
         cv::Mat & boundary,
         cv::Mat & labels,
         bool sz);

void ucm2seg(const cv::Mat & ucm_mtr,
             cv::Mat & boundary,
             cv::Mat & labels,
             double thres,
             bool sz);

}
