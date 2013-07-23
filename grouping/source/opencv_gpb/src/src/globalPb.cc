//
//    globalPb
//
//    Created by Di Yang, Vicent Rabaud, and Gary Bradski on 31/05/13.
//    Copyright (c) 2013 The Australian National University. 
//    and Willow Garage inc.
//    All rights reserved.
//    
//

#include "cv_lib_filters.hh"
#include "globalPb.hh"
#include "buildW.hh"
#include <string>
#include <sstream>

using namespace std;
using namespace libFilters;

namespace
{ static double* 
  _gPb_Weights(int nChannels)
  {
    double *weights = new double[13];
    if(nChannels == 3){
      weights[0] = 0.0;    weights[1] = 0.0;    weights[2] = 0.0039;
      weights[3] = 0.0050; weights[4] = 0.0058; weights[5] = 0.0069;
      weights[6] = 0.0040; weights[7] = 0.0044; weights[8] = 0.0049;
      weights[9] = 0.0024; weights[10]= 0.0027; weights[11]= 0.0170;
      weights[12]= 0.0074;
    }else{
      weights[0] = 0.0;    weights[1] = 0.0;    weights[2] = 0.0054;
      weights[3] = 0.0;    weights[4] = 0.0;    weights[5] = 0.0;
      weights[6] = 0.0;    weights[7] = 0.0;    weights[8] = 0.0;
      weights[9] = 0.0048; weights[10]= 0.0049; weights[11]= 0.0264;
      weights[12]= 0.0090;
    }
    return weights;
  }

  static double* 
  _mPb_Weights(int nChannels)
  {
    double *weights = new double[13];
    if(nChannels == 3){
      weights[0] = 0.0146; weights[1] = 0.0145; weights[2] = 0.0163;
      weights[3] = 0.0210; weights[4] = 0.0243; weights[5] = 0.0287;
      weights[6] = 0.0166; weights[7] = 0.0185; weights[8] = 0.0204;
      weights[9] = 0.0101; weights[10]= 0.0111; weights[11]= 0.0141;
    }else{
      weights[0] = 0.0245; weights[1] = 0.0220; weights[2] = 0.0;
      weights[3] = 0.0;    weights[4] = 0.0;    weights[5] = 0.0;
      weights[6] = 0.0;    weights[7] = 0.0;    weights[8] = 0.0;
      weights[9] = 0.0208; weights[10]= 0.0210; weights[11]= 0.0229;
    }
    return weights;
  }
}

namespace cv
{
  void
  pb_parts_final_selected(vector<cv::Mat> & layers,
			  cv::Mat & texton,
			  vector<cv::Mat> & bg_r3,
			  vector<cv::Mat> & bg_r5,
			  vector<cv::Mat> & bg_r10,
			  vector<cv::Mat> & cga_r5,
			  vector<cv::Mat> & cga_r10,
			  vector<cv::Mat> & cga_r20,
			  vector<cv::Mat> & cgb_r5,
			  vector<cv::Mat> & cgb_r10,
			  vector<cv::Mat> & cgb_r20,
			  vector<cv::Mat> & tg_r5,
			  vector<cv::Mat> & tg_r10,
			  vector<cv::Mat> & tg_r20)
  {
    int n_ori      = 8;                       // number of orientations
    int num_bins   = 25;                      // bins for L, b, a
    int Kmean_bins = 64;                      // bins for texton
    double bg_smooth_sigma = 0.1;             // bg histogram smoothing sigma
    double cg_smooth_sigma = 0.05;            // cg histogram smoothing sigma
    double sigma_tg_filt_sm = 2.0;            // sigma for small tg filters
    double sigma_tg_filt_lg = sqrt(2.0)*2.0;  // sigma for large tg filters
    
    int n_bg = 3;
    int n_cg = 3;
    int n_tg = 3;
    int r_bg[3] = { 3,  5, 10 };
    int r_cg[3] = { 5, 10, 20 };
    int r_tg[3] = { 5, 10, 20 };
    
    cv::Mat color, grey, ones;
    cv::Mat bg_smooth_kernel, cg_smooth_kernel;
    cv::merge(layers, color);
    cv::cvtColor(color, grey, CV_BGR2GRAY);
    ones = cv::Mat::ones(color.rows, color.cols, CV_32FC1);
    
    // Histogram filter generation
    gaussianFilter1D(double(num_bins)*bg_smooth_sigma, 0, false, bg_smooth_kernel);
    gaussianFilter1D(double(num_bins)*cg_smooth_sigma, 0, false, cg_smooth_kernel);
    
    // Normalize color channels
    color.convertTo(color, CV_32FC3);
    cv::split(color, layers);
    for(size_t c=0; c<3; c++)
      cv::multiply(layers[c], ones, layers[c], 1.0/255.0);
    cv::merge(layers, color);
    
    // Color convert, including gamma correction
    cv::cvtColor(color, color, CV_BGR2Lab);

    // Normalize Lab channels
    cv::split(color, layers);
    for(size_t c=0; c<3; c++)
      for(size_t i=0; i<layers[c].rows; i++){
	for(size_t j=0; j<layers[c].cols; j++){
	  if(c==0)
	    layers[c].at<float>(i,j) = layers[c].at<float>(i,j)/100.0;
      	  else
	    layers[c].at<float>(i,j) = (layers[c].at<float>(i,j)+73.0)/168.0;
	  if(layers[c].at<float>(i,j) < 0.0)
	    layers[c].at<float>(i,j) = 0.0;
	  else if(layers[c].at<float>(i,j) > 1.0)
	    layers[c].at<float>(i,j) = 1.0;

	  //quantize color channels
	  
	  float bin = floor(layers[c].at<float>(i,j)*float(num_bins));
	  if(bin == float(num_bins)) bin--;
	  layers[c].at<float>(i,j)=bin;
	}
      }

    /********* END OF FILTERS INTIALIZATION ***************/

    cout<<"computing texton ... "<<endl;
    textonRun(grey, texton, n_ori, Kmean_bins, sigma_tg_filt_sm, sigma_tg_filt_lg);

    // L Channel
    cout<<"computing bg's ... "<<endl;
    gradient_hist_2D(layers[0], r_bg[0], n_ori, num_bins, bg_smooth_kernel, bg_r3);
    gradient_hist_2D(layers[0], r_bg[1], n_ori, num_bins, bg_smooth_kernel, bg_r5);
    gradient_hist_2D(layers[0], r_bg[2], n_ori, num_bins, bg_smooth_kernel, bg_r10);

    // a Channel
    cout<<"computing cga's ... "<<endl;
    gradient_hist_2D(layers[1], r_cg[0], n_ori, num_bins, cg_smooth_kernel, cga_r5);
    gradient_hist_2D(layers[1], r_cg[1], n_ori, num_bins, cg_smooth_kernel, cga_r10);
    gradient_hist_2D(layers[1], r_cg[2], n_ori, num_bins, cg_smooth_kernel, cga_r20);

    // b Channel
    cout<<"computing cgb's ... "<<endl;
    gradient_hist_2D(layers[2], r_cg[0], n_ori, num_bins, cg_smooth_kernel, cgb_r5);
    gradient_hist_2D(layers[2], r_cg[1], n_ori, num_bins, cg_smooth_kernel, cgb_r10);
    gradient_hist_2D(layers[2], r_cg[2], n_ori, num_bins, cg_smooth_kernel, cgb_r20);

    // T channel
    cout<<"computing tg's ... "<<endl;
    gradient_hist_2D(texton, r_tg[0], n_ori, Kmean_bins, tg_r5);
    gradient_hist_2D(texton, r_tg[1], n_ori, Kmean_bins, tg_r10);
    gradient_hist_2D(texton, r_tg[2], n_ori, Kmean_bins, tg_r20);
    
    texton.convertTo(texton, CV_8UC1);
  }
  
  void
  nonmax_oriented_2D(const cv::Mat & mPb_max,
		     const cv::Mat & index,
		     cv::Mat & output,
		     double o_tol)
  {
    int rows = mPb_max.rows;
    int cols = mPb_max.cols;
    //mPb_max.copyTo(output);
    output=cv::Mat::zeros(rows, cols, CV_32FC1);
    for(size_t i=0; i<rows; i++)
      for(size_t j=0; j<cols; j++){
	double ori = index.at<float>(i,j);
	double theta = ori; //ori+M_PI/2.0;
	theta -= double(int(theta/M_PI))*M_PI;
	double v = mPb_max.at<float>(i,j);
	int ind0a_x = 0, ind0a_y = 0, ind0b_x = 0, ind0b_y = 0; 
	int ind1a_x = 0, ind1a_y = 0, ind1b_x = 0, ind1b_y = 0;
	double d = 0;
	bool valid0 = false, valid1 = false;
	double theta2 = 0;
	
	if(theta < 1e-6)
	  theta = 0.0;

	if(theta == 0){
	  valid0 = (i>0); valid1 = (i<(rows-1));
	  if(valid0){ ind0a_x = i-1; ind0a_y = j; ind0b_x = i-1; ind0b_y = j; }
	  if(valid1){ ind1a_x = i+1; ind1a_y = j; ind1b_x = i+1; ind1b_y = j; }
	}else if(theta < M_PI/4.0){
	  d = tan(theta);
	  theta2 = theta;
	  valid0 = ((i>0) && (j>0)); 
	  valid1 = (i<(rows-1) && j<(cols-1));
	  if(valid0){ ind0a_x = i-1; ind0a_y = j; ind0b_x = i-1; ind0b_y = j-1; }
	  if(valid1){ ind1a_x = i+1; ind1a_y = j; ind1b_x = i+1; ind1b_y = j+1; }
	}else if(theta < M_PI/2.0){
	  d = tan(M_PI/2.0 - theta);
	  theta2 = M_PI/2.0 - theta;
	  valid0 = ((i>0) && (j>0)); 
	  valid1 = (i<(rows-1) && j<(cols-1));
	  if(valid0){ ind0a_x = i; ind0a_y = j-1; ind0b_x = i-1; ind0b_y = j-1; }
	  if(valid1){ ind1a_x = i; ind1a_y = j+1; ind1b_x = i+1; ind1b_y = j+1; }
	}else if(theta == M_PI/2.0){
	  valid0 = (j>0); valid1 = (j<(cols-1));
	  if(valid0){ ind0a_x = i; ind0a_y = j-1; ind0b_x = i; ind0b_y = j-1; }
	  if(valid1){ ind1a_x = i; ind1a_y = j+1; ind1b_x = i; ind1b_y = j+1; }
	}else if(theta < 3.0*M_PI/4.0){
	  d = tan(theta - M_PI/2.0);
	  theta2 = theta - M_PI/2.0;
	  valid0 = ((i<rows-1) && (j>0)); 
	  valid1 = (i>0 && j<(cols-1));
	  if(valid0){ ind0a_x = i; ind0a_y = j-1; ind0b_x = i+1; ind0b_y = j-1; }
	  if(valid1){ ind1a_x = i; ind1a_y = j+1; ind1b_x = i-1; ind1b_y = j+1; }
	}else{
	  d = tan(M_PI-theta);
	  theta2 = M_PI-theta;
	  valid0 = ((i<rows-1) && (j>0)); 
	  valid1 = (i>0 && j<(cols-1));
	  if(valid0){ ind0a_x = i+1; ind0a_y = j; ind0b_x = i+1; ind0b_y = j-1; }
	  if(valid1){ ind1a_x = i-1; ind1a_y = j; ind1b_x = i-1; ind1b_y = j+1; }
	}

	if(d > 1.0 || d < 0.0)
	  cout<<"something wrong"<<endl;

	if(valid0 && valid1){
	  double v0a = 0, v0b = 0, v1a = 0, v1b = 0;
	  double ori0a = 0, ori0b = 0, ori1a = 0, ori1b = 0;
	  if(valid0){
	    v0a = mPb_max.at<float>(ind0a_x, ind0a_y);
	    v0b = mPb_max.at<float>(ind0b_x, ind0b_y);
	    ori0a = index.at<float>(ind0a_x, ind0a_y)-ori;
	    ori0b = index.at<float>(ind0b_x, ind0b_y)-ori;
	  }
	  if(valid1){
	    v1a = mPb_max.at<float>(ind1a_x, ind1a_y);
	    v1b = mPb_max.at<float>(ind1b_x, ind1b_y);
	    ori1a = index.at<float>(ind1a_x, ind1a_y)-ori;
	    ori1b = index.at<float>(ind1b_x, ind1b_y)-ori;
	  }
	  ori0a -= double(int(ori0a/(2*M_PI))) * (2*M_PI);
	  ori0b -= double(int(ori0b/(2*M_PI))) * (2*M_PI);
	  ori1a -= double(int(ori1a/(2*M_PI))) * (2*M_PI);
	  ori1b -= double(int(ori1b/(2*M_PI))) * (2*M_PI);
	  if(ori0a >= M_PI) {ori0a = 2*M_PI - ori0a; }
	  if(ori0b >= M_PI) {ori0b = 2*M_PI - ori0b; }
	  if(ori1a >= M_PI) {ori1a = 2*M_PI - ori1a; }
	  if(ori1b >= M_PI) {ori1b = 2*M_PI - ori1b; }
	  if(ori0a >= M_PI/2.0) {ori0a = M_PI - ori0a; }
	  if(ori0b >= M_PI/2.0) {ori0b = M_PI - ori0b; }
	  if(ori1a >= M_PI/2.0) {ori1a = M_PI - ori1a; }
	  if(ori1b >= M_PI/2.0) {ori1b = M_PI - ori1b; }

	  ori0a = (ori0a <= o_tol) ? 0.0 : (ori0a - o_tol);
	  ori0b = (ori0b <= o_tol) ? 0.0 : (ori0b - o_tol);
	  ori1a = (ori1a <= o_tol) ? 0.0 : (ori1a - o_tol);
	  ori1b = (ori1b <= o_tol) ? 0.0 : (ori1b - o_tol);
	  
	  double v0 = (1.0-d)*v0a*cos(ori0a) + d*v0b*cos(ori0b);
	  double v1 = (1.0-d)*v1a*cos(ori1a) + d*v1b*cos(ori1b);
	  if((v>v0) && (v>v1)){
	    v = 1.2*v;
	    if(v > 1.0) v = 1.0;
	    if(v < 0.0) v = 0.0;
	    output.at<float>(i, j) = v;
	  }
	}
      }
  }

  void 
  MakeFilter(const int radii,
	     const double theta,
	     cv::Mat & kernel)
  {
    double ra, rb, ira2, irb2;
    double sint, cost, ai, bi;
    double x[5] = {0};
    int wr;
    cv::Mat A = cv::Mat::zeros(3, 3, CV_32FC1);
                  cv::Mat y = cv::Mat::zeros(3, 1, CV_32FC1);
    ra = MAX(1.5, double(radii));
    rb = MAX(1.5, double(radii)/4);
    ira2 = 1.0/(pow(ra, 2));
    irb2 = 1.0/(pow(rb, 2));
    wr = int(MAX(ra, rb));
    kernel = cv::Mat::zeros(2*wr+1, 2*wr+1, CV_32FC1);
  
    sint = sin(theta);
    cost = cos(theta);
    for(size_t i = 0; i <= 2*wr; i++)
      for(size_t j = 0; j <= 2*wr; j++){
	ai = -(double(i)-double(wr))*sint + (double(j)-double(wr))*cost;
	bi =  (double(i)-double(wr))*cost + (double(j)-double(wr))*sint;
	if((ai*ai*ira2 + bi*bi*irb2) > 1) continue;
	for(size_t n=0; n < 5; n++)
	  x[n] += pow(ai, double(n));
      }
    for(size_t i=0; i < 3; i++)
      for(size_t j = i; j < i+3; j++){
	A.at<float>(i, j-i) = x[j];
      }
    A = A.inv(DECOMP_SVD);
    for(size_t i = 0; i <= 2*wr; i++)
      for(size_t j = 0; j <= 2*wr; j++){
	ai = -(double(i)-double(wr))*sint + (double(j)-double(wr))*cost;
	bi =  (double(i)-double(wr))*cost + (double(j)-double(wr))*sint;
	if((ai*ai*ira2 + bi*bi*irb2) > 1) continue;
	for(size_t n=0; n < 3; n++)
	  y.at<float>(n,0) = pow(ai, double(n));
	y = A*y;
	kernel.at<float>(j,i) = y.at<float>(0,0);
      }
  }

  void
  multiscalePb(const cv::Mat & image,
	       cv::Mat & mPb_max,
	       vector<cv::Mat> & bg_r3,
	       vector<cv::Mat> & bg_r5,
	       vector<cv::Mat> & bg_r10,
	       vector<cv::Mat> & cga_r5,
	       vector<cv::Mat> & cga_r10,
	       vector<cv::Mat> & cga_r20,
	       vector<cv::Mat> & cgb_r5,
	       vector<cv::Mat> & cgb_r10,
	       vector<cv::Mat> & cgb_r20,
	       vector<cv::Mat> & tg_r5,
	       vector<cv::Mat> & tg_r10,
	       vector<cv::Mat> & tg_r20)
  {
    cv::Mat texton, kernel, angles, temp;
    vector<cv::Mat> layers, mPb_all;
    int n_ori = 8;
    int radii[4] ={3, 5, 10, 20};
    double* weights, *ori;
    
    weights = _mPb_Weights(image.channels());
    layers.resize(3); 
    if(image.channels() == 3)
      cv::split(image, layers);
    else
      for(size_t i=0; i<3; i++)
	image.copyTo(layers[i]);
    
    pb_parts_final_selected(layers, texton, bg_r3, bg_r5, bg_r10, cga_r5, cga_r10, cga_r20, cgb_r5, cgb_r10, cgb_r20, tg_r5, tg_r10, tg_r20);
    
    cout<<"Cues smoothing ..."<<endl;

    mPb_all.resize(n_ori);
    ori = standard_filter_orientations(n_ori, RAD);
    for(size_t idx=0; idx<n_ori; idx++){
      // radian 3
      MakeFilter(radii[0], ori[idx], kernel);
      cv::filter2D(bg_r3[idx],   bg_r3[idx],   CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);
      
      // radian 5
      MakeFilter(radii[1], ori[idx], kernel);
      cv::filter2D(bg_r5[idx],   bg_r5[idx],   CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);
      cv::filter2D(cga_r5[idx],  cga_r5[idx],  CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);
      cv::filter2D(cgb_r5[idx],  cgb_r5[idx],  CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);
      cv::filter2D(tg_r5[idx],   tg_r5[idx],   CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);

      // radian 10
      MakeFilter(radii[2], ori[idx], kernel);
      cv::filter2D(bg_r10[idx],  bg_r10[idx],  CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);
      cv::filter2D(cga_r10[idx], cga_r10[idx], CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);
      cv::filter2D(cgb_r10[idx], cgb_r10[idx], CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);
      cv::filter2D(tg_r10[idx],  tg_r10[idx],  CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);

      // radian 20
      MakeFilter(radii[3], ori[idx], kernel);
      cv::filter2D(cga_r20[idx], cga_r20[idx], CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);
      cv::filter2D(cgb_r20[idx], cgb_r20[idx], CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);
      cv::filter2D(tg_r20[idx],  tg_r20[idx],  CV_32F, kernel, cv::Point(-1, -1), 0.0, cv::BORDER_REFLECT);
      
      mPb_all[idx] = cv::Mat::zeros(image.rows, image.cols, CV_32FC1);
      addWeighted(mPb_all[idx], 1.0, bg_r3[idx],   weights[0], 0.0, mPb_all[idx]);
      addWeighted(mPb_all[idx], 1.0, bg_r5[idx],   weights[1], 0.0, mPb_all[idx]);
      addWeighted(mPb_all[idx], 1.0, bg_r10[idx],  weights[2], 0.0, mPb_all[idx]);
      addWeighted(mPb_all[idx], 1.0, cga_r5[idx],  weights[3], 0.0, mPb_all[idx]);
      addWeighted(mPb_all[idx], 1.0, cga_r10[idx], weights[4], 0.0, mPb_all[idx]);
      addWeighted(mPb_all[idx], 1.0, cga_r20[idx], weights[5], 0.0, mPb_all[idx]);
      addWeighted(mPb_all[idx], 1.0, cgb_r5[idx],  weights[6], 0.0, mPb_all[idx]);
      addWeighted(mPb_all[idx], 1.0, cgb_r10[idx], weights[7], 0.0, mPb_all[idx]);
      addWeighted(mPb_all[idx], 1.0, cgb_r20[idx], weights[8], 0.0, mPb_all[idx]);
      addWeighted(mPb_all[idx], 1.0, tg_r5[idx],   weights[9], 0.0, mPb_all[idx]);
      addWeighted(mPb_all[idx], 1.0, tg_r10[idx],  weights[10], 0.0, mPb_all[idx]);
      addWeighted(mPb_all[idx], 1.0, tg_r20[idx],  weights[11], 0.0, mPb_all[idx]);
      
      if(idx == 0){
	angles = cv::Mat::ones(image.rows, image.cols, CV_32FC1);
	cv::multiply(angles, angles, angles, ori[idx]);
	mPb_all[idx].copyTo(mPb_max);
      }
      else
	for(size_t i=0; i<image.rows; i++)
	  for(size_t j=0; j<image.cols; j++)
	    if(mPb_max.at<float>(i,j) < mPb_all[idx].at<float>(i,j)){
	      angles.at<float>(i,j) = ori[idx];
	      mPb_max.at<float>(i,j) = mPb_all[idx].at<float>(i,j);
	    }
    }
    nonmax_oriented_2D(mPb_max, angles, temp, M_PI/8.0);
    temp.copyTo(mPb_max);
  }
  
  void gPb_gen(const cv::Mat & mPb_max,
	       const double* weights,
	       const vector<cv::Mat> & sPb,
	       const vector<cv::Mat> & bg_r3,
	       const vector<cv::Mat> & bg_r5,
	       const vector<cv::Mat> & bg_r10,
	       const vector<cv::Mat> & cga_r5,
	       const vector<cv::Mat> & cga_r10,
	       const vector<cv::Mat> & cga_r20,
	       const vector<cv::Mat> & cgb_r5,
	       const vector<cv::Mat> & cgb_r10,
	       const vector<cv::Mat> & cgb_r20,
	       const vector<cv::Mat> & tg_r5,
	       const vector<cv::Mat> & tg_r10,
	       const vector<cv::Mat> & tg_r20,
	       vector<cv::Mat> & gPb_ori,
	       cv::Mat & gPb_thin,
	       cv::Mat & gPb
	       )
  {
    cv::Mat img_tmp, eroded, temp, bwskel;
    int n_ori = 8, nnz = 0;
    
    gPb_ori.resize(n_ori);
    for(size_t idx=0; idx<n_ori; idx++){
      gPb_ori[idx] = cv::Mat::zeros(mPb_max.rows, mPb_max.cols, CV_32FC1);
      addWeighted(gPb_ori[idx], 1.0, bg_r3[idx],   weights[0], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, bg_r5[idx],   weights[1], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, bg_r10[idx],  weights[2], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, cga_r5[idx],  weights[3], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, cga_r10[idx], weights[4], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, cga_r20[idx], weights[5], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, cgb_r5[idx],  weights[6], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, cgb_r10[idx], weights[7], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, cgb_r20[idx], weights[8], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, tg_r5[idx],   weights[9], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, tg_r10[idx],  weights[10], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, tg_r20[idx],  weights[11], 0.0, gPb_ori[idx]);
      addWeighted(gPb_ori[idx], 1.0, sPb[idx], weights[12], 0.0, gPb_ori[idx]);
    
      if(idx == 0)
	gPb_ori[idx].copyTo(gPb);
      else
	for(size_t i=0; i<mPb_max.rows; i++)
	  for(size_t j=0; j<mPb_max.cols; j++)
	    if(gPb.at<float>(i,j) < gPb_ori[idx].at<float>(i,j))
	      gPb.at<float>(i,j) = gPb_ori[idx].at<float>(i,j);	    
    }
    
    gPb.copyTo(gPb_thin);
    for(size_t i=0; i<mPb_max.rows; i++)
      for(size_t j=0; j<mPb_max.cols; j++)
	if(mPb_max.at<float>(i,j)<0.05)
	  gPb_thin.at<float>(i,j) = 0.0;

    bwskel = cv::Mat::zeros(mPb_max.rows, mPb_max.cols, CV_32FC1);
    gPb_thin.copyTo(img_tmp);
    do{
      cv::erode(img_tmp, eroded, cv::Mat(), cv::Point(-1, -1));
      cv::dilate(eroded, temp, cv::Mat(), cv::Point(-1,-1));
      cv::subtract(img_tmp, temp, temp);
      nnz = 0;
      for(size_t i=0; i<mPb_max.rows; i++)
	for(size_t j=0; j<mPb_max.cols; j++){
	  if(bwskel.at<float>(i,j) > 0.0 || temp.at<float>(i,j) > 0.0)
	    bwskel.at<float>(i,j) = 1.0;
	  else
	    bwskel.at<float>(i,j) = 0.0;
	  if(eroded.at<float>(i,j) != 0.0) nnz++;
	}
      eroded.copyTo(img_tmp);
    }while(nnz);
    cv::multiply(gPb_thin, bwskel, gPb_thin, 1.0);
  }

  void sPb_gen(cv::Mat & mPb_max,
	   vector<cv::Mat> & sPb)
  {
    int n_ori = 8;
    sPb.resize(n_ori);
    /*cv::Mat ind_x, ind_y, val;
    cv::buildW(mPb_max, ind_x, ind_y, val);
    cv::Mat diag = cv::Mat::zeros(mPb_max.rows*mPb_max.cols, 1, CV_32FC1);
    
    // compute Diagnoal matrix D based on affinity matrix
    for(size_t i=0; i<ind_y.rows; i++){
      int idx = ind_y.at<int>(i,1);
      diag.at<float>(idx,1) += val.at<float>(i,1);
      }*/

    //Generate inv(D)*(D-W) for computing generalized eigenvalues and eigenvectors
    
    /*for(size_t i=0; i<ind_x.rows; i++ ){
      int idx = ind_x.at<int>(i,1);
      double temp_diag = diag.at<float>(idx, 1);
      if(idx == ind_y.at<int>(i,1))
	val.at<float>(i,1) = (temp_diag-val.at<float>(i,1))/temp_diag;
      else
	val.at<float>(i,1) = -val.at<float>(i,1)/temp_diag;    
	}*/

    // sPb eigen decomposition problem will be solved later. This is a cheating code for ucm development. 
    FILE* pFile1, *pFile2, *pFile3, *pFile4, *pFile5, *pFile6, *pFile7, *pFile8;
    pFile1 = fopen("sPb_data/sPb_l1.txt", "r");
    pFile2 = fopen("sPb_data/sPb_l2.txt", "r");
    pFile3 = fopen("sPb_data/sPb_l3.txt", "r");
    pFile4 = fopen("sPb_data/sPb_l4.txt", "r");
    pFile5 = fopen("sPb_data/sPb_l5.txt", "r");
    pFile6 = fopen("sPb_data/sPb_l6.txt", "r");
    pFile7 = fopen("sPb_data/sPb_l7.txt", "r");
    pFile8 = fopen("sPb_data/sPb_l8.txt", "r");
    for(size_t idx=0; idx<n_ori; idx++)
      sPb[idx] = cv::Mat::zeros(mPb_max.rows, mPb_max.cols, CV_32FC1);
    
    for(size_t i=0; i<mPb_max.rows; i++)
      for(size_t j=0; j<mPb_max.cols; j++){
	float l1, l2, l3, l4, l5, l6, l7, l8;
	fscanf(pFile1, "%f", &l1);
	fscanf(pFile2, "%f", &l2);
	fscanf(pFile3, "%f", &l3);
	fscanf(pFile4, "%f", &l4);
	fscanf(pFile5, "%f", &l5);
	fscanf(pFile6, "%f", &l6);
	fscanf(pFile7, "%f", &l7);
	fscanf(pFile8, "%f", &l8);
	
	sPb[0].at<float>(i, j) = l1;
	sPb[1].at<float>(i, j) = l2;
	sPb[2].at<float>(i, j) = l3;
	sPb[3].at<float>(i, j) = l4;
	sPb[4].at<float>(i, j) = l5;
	sPb[5].at<float>(i, j) = l6;
	sPb[6].at<float>(i, j) = l7;
	sPb[7].at<float>(i, j) = l8;
      } 
    fclose(pFile1);
    fclose(pFile2);
    fclose(pFile3);
    fclose(pFile4);
    fclose(pFile5);
    fclose(pFile6);
    fclose(pFile7);
    fclose(pFile8);    
  }

  void 
  globalPb(const cv::Mat & image,
	   cv::Mat & gPb,
	   cv::Mat & gPb_thin,
	   vector<cv::Mat> & gPb_ori)
  {
    gPb = cv::Mat::zeros(image.rows, image.cols, CV_32FC1);
    cv::Mat mPb_max;
    vector<cv::Mat> bg_r3, bg_r5, bg_r10, cga_r5, cga_r10, cga_r20; 
    vector<cv::Mat> cgb_r5, cgb_r10, cgb_r20, tg_r5, tg_r10, tg_r20, sPb;
    double *weights;
    weights = _gPb_Weights(image.channels());

    //multiscalePb - mPb
    multiscalePb(image, mPb_max, bg_r3,bg_r5, bg_r10, cga_r5, cga_r10, cga_r20, cgb_r5, cgb_r10, cgb_r20, tg_r5, tg_r10, tg_r20);
    mPb_max.copyTo(gPb);
    
    //spectralPb   - sPb
    sPb_gen(mPb_max, sPb);
    
    //globalPb - gPb
    gPb_gen(mPb_max, weights, sPb, bg_r3,bg_r5, bg_r10, cga_r5, cga_r10, cga_r20, cgb_r5, cgb_r10, cgb_r20, tg_r5, tg_r10, tg_r20, gPb_ori, gPb_thin, gPb);
  }
}