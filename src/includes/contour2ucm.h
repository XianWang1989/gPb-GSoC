#include <iostream>
#include <vector>
#include <stack>
#include <math.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include "watershed.h"
#include "ucm_mean_pb.h"
#include "uvt.h"

class contour_vertex;
class contour_edge;

typedef std::shared_ptr<contour_vertex> ptr_contour_vertex;
typedef std::shared_ptr<contour_edge> ptr_contour_edge;

class contour_vertex {
public:
    int id;
    bool is_subdivision;
    int x;
    int y;
    std::vector<ptr_contour_edge> edges_start;
    std::vector<ptr_contour_edge> edges_end;

    //------- vertex method ---------

    cv::Point point() const;
};

class contour_edge {
public:
    int id;
    int contour_equiv_id;
    bool is_completion;
    std::vector<int> x_coords;
    std::vector<int> y_coords;
    ptr_contour_vertex vertex_start;
    ptr_contour_vertex vertex_end;
    int vertex_start_enum;
    int vertex_end_enum;

    //------- edge method -----------

    int size() const;
    double length() const;
    cv::Point point(int) const;
};



namespace cv
{
void contour2ucm(const cv::Mat & gPb,
                 const std::vector<cv::Mat> & gPb_ori,
                 cv::Mat & ucm,
                 bool label);
}
