#include "globalPb.hh"
#include "cv_lib_filters.hh"
#include "contour2ucm.hh"

using namespace std;
using namespace libFilters;

cv::Mat markers, ucm;
cv::Point prev_pt(-1, -1);
void on_mouse( int event, int x, int y, int flags, void* param )
{
  if( ucm.empty() )
    return;
	
  if( event == cv::EVENT_LBUTTONUP || !(flags& cv::EVENT_FLAG_LBUTTON) )
    prev_pt = cv::Point(-1,-1);
  else if( event == cv::EVENT_LBUTTONDOWN )
    prev_pt = cv::Point(x,y);
  else if( event == cv::EVENT_MOUSEMOVE && (flags & cv::EVENT_FLAG_LBUTTON) )
  {
    cv::Point pt = cv::Point(x,y);
    if( prev_pt.x < 0 )
      prev_pt = pt;
    cv::line( markers, prev_pt, pt, uchar(255), 3, 8, 0 );
    cv::line( ucm,     prev_pt, pt, 1.0, 3, 8, 0 );
    prev_pt = pt;
    cv::imshow("ucm", ucm );
  }
}

int main(int argc, char** argv){

  //info block
  system("clear");
  cout<<"(before running it, roughly mark the areas on the ucm window)"<<endl;
  cout<<"Press 'r' - resort the original ucm, and remark"<<endl;
  cout<<"Press 'w' or 'ENTER' - conduct interactive segmentation"<<endl;
  cout<<"Press 'ESC' - exit the program"<<endl<<endl<<endl;

  cv::Mat img0, gPb, gPb_thin, ucm2, boundary, labels, seeds;
  vector<cv::Mat> gPb_ori; 

  img0 = cv::imread(argv[1], CV_LOAD_IMAGE_COLOR);
  cv::globalPb(img0, gPb, gPb_thin, gPb_ori);
  cv::contour2ucm(gPb, gPb_ori, ucm, DOUBLE_SIZE);
  
  //back up
  ucm.copyTo(ucm2);
  markers = cv::Mat::zeros(ucm.size(), CV_8UC1);
  
  cv::imshow("Original", img0);
  cv::imshow("gPb",  gPb);
  cv::imshow("gPb_thin", gPb_thin);
  cv::imshow("ucm", ucm2);
  cv::setMouseCallback("ucm", on_mouse, 0);

  while(true){
    char ch = cv::waitKey(0);
    if(ch == 27) break;
    
    if(ch == 'r'){
      //restore everything
      markers = cv::Mat::zeros(markers.size(), CV_8UC1);
      ucm.copyTo(ucm2);
    }

    if(ch == 'w' || ch == '\n'){
      vector< vector<cv::Point> > contours;
      vector<cv::Vec4i> hierarchy;
      cv::findContours(markers, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
      seeds = cv::Mat::zeros(markers.size(), CV_8UC1);
      for( int i = 0, color = 1; i< contours.size(); i++ ){ 
	cv::drawContours(seeds, contours, i, uchar(color++), -1, 8, hierarchy, 0, cv::Point() );
      }
      seeds.convertTo(seeds, CV_32SC1);
      cv::uvt(ucm, seeds, boundary, labels, SINGLE_SIZE);
      cv::imshow("boundary", boundary*255);
      cv::imshow("labels", labels*127);
    }
      
  }
  
}
