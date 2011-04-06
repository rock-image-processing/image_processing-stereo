/*
Copyright 2011. All rights reserved.
Institute of Measurement and Control Systems
Karlsruhe Institute of Technology, Germany

This file is part of libelas.
Authors: Andreas Geiger, Jan F.

libelas is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation; either version 3 of the License, or any later version.

libelas is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
libelas; if not, write to the Free Software Foundation, Inc., 51 Franklin
Street, Fifth Floor, Boston, MA 02110-1301, USA 
*/

// program showing how libelas can be used, try "./elas -h" for help

#include "densestereo.h"

using namespace std;

DenseStereo::DenseStereo() {
  // configure Elas and instantiate it
  Elas::parameters param;
  param.postprocess_only_left = false;
  elas = new Elas(param);
}

DenseStereo::~DenseStereo() {
  delete elas;
}

// rectify images with opencv
void DenseStereo::rectify(IplImage *image, const bool right_image){
  ImageProcessing *imgproc = new ImageProcessing();// for reading and rectifying the image
  
  // load calibration parameters
  CalibrationParameters *cp = new CalibrationParameters();
  cp->loadParameters();
  cp->calculateUndistortAndRectifyMaps();

  // rectify image
  int result = imgproc->preprocessImage(image, right_image, cp);
  if(result != 0)
    {
      std::cerr << "Error preprocessing image." << std::endl;
      //throw exception?
    }
    
  delete imgproc;
  delete cp;
}

// compute disparities of image input pair left_frame, right_frame
void DenseStereo::process_FramePair (const cv::Mat &left_frame,const cv::Mat &right_frame,
				     cv::Mat &left_output_frame,cv::Mat &right_output_frame) {

  // rectify and convert images to Grayscale and to image<uchar>
  image<uchar> *I1,*I2;
  IplImage left = left_frame;
  IplImage right = right_frame;
  rectify(&left,0);
  rectify(&right,1);
  I1 = cvtCvMatToGrayscaleImage(&left);
  I2 = cvtCvMatToGrayscaleImage(&right);
  
  // check for correct size
  if (I1->width()<=0 || I1->height() <=0 || I2->width()<=0 || I2->height() <=0 ||
      I1->width()!=I2->width() || I1->height()!=I2->height()) {
    cerr << "ERROR: Images must be of same size, but" << endl;
    cerr << "       I1: " << I1->width() <<  " x " << I1->height() << 
                 ", I2: " << I2->width() <<  " x " << I2->height() << endl;
    delete I1;
    delete I2;
    //TODO throw exception?
    return;
  }

  // get image width and height
  int32_t width  = I1->width();
  int32_t height = I1->height();

  // allocate memory for disparity images
  const int32_t dims[3] = {width,height,width}; // bytes per line = width
  float* D1_data = (float*)malloc(width*height*sizeof(float));
  float* D2_data = (float*)malloc(width*height*sizeof(float));
  
  //process
  elas->process(I1->data,I2->data,D1_data,D2_data,dims);

  // find maximum disparity for scaling output disparity images to [0..255]
  //TODO really needed?, probabely not
  float disp_max = 0;
  for (int32_t i=0; i<width*height; i++) {
    if (D1_data[i]>disp_max) disp_max = D1_data[i];
    if (D2_data[i]>disp_max) disp_max = D2_data[i];
  }

  // copy float to uchar
  image<uchar> *D1 = new image<uchar>(width,height);
  image<uchar> *D2 = new image<uchar>(width,height);
  for (int32_t i=0; i<width*height; i++) {
    D1->data[i] = (uint8_t)max(255.0*D1_data[i]/disp_max,0.0);
    D2->data[i] = (uint8_t)max(255.0*D2_data[i]/disp_max,0.0);
  }

  // convert back to openCV cv::Mat
  left_output_frame = convertImage2CvMat(D1);
  right_output_frame = convertImage2CvMat(D2);

  // save disparity image with openCV to test if conversion is ok
  //imwrite("opencv_disparity.png",left_output_frame);
  
  // save disparity images
  /*string output_l = "left";
  string output_r = "right";
  output_l += "_disp.pgm";
  output_r += "_disp.pgm";
  savePGM(D1,output_l.c_str());
  savePGM(D2,output_r.c_str());*/
  
  // free memory
  delete I1;
  delete I2;
  delete D1;
  delete D2;
  free(D1_data);
  free(D2_data);
}

// compute disparities of image input pair file_1, file_2
void DenseStereo::process_images (const char* file_1,const char* file_2) {

  cout << "Processing: " << file_1 << ", " << file_2 << endl;

  // load images
  image<uchar> *I1,*I2;
  I1 = loadImage(file_1,0);
  I2 = loadImage(file_2,1);
  
  // check for correct size
  if (I1->width()<=0 || I1->height() <=0 || I2->width()<=0 || I2->height() <=0 ||
      I1->width()!=I2->width() || I1->height()!=I2->height()) {
    cout << "ERROR: Images must be of same size, but" << endl;
    cout << "       I1: " << I1->width() <<  " x " << I1->height() << 
                 ", I2: " << I2->width() <<  " x " << I2->height() << endl;
    delete I1;
    delete I2;
    return;    
  }

  // get image width and height
  int32_t width  = I1->width();
  int32_t height = I1->height();

  // allocate memory for disparity images
  const int32_t dims[3] = {width,height,width}; // bytes per line = width
  float* D1_data = (float*)malloc(width*height*sizeof(float));
  float* D2_data = (float*)malloc(width*height*sizeof(float));

  //process
  Elas::parameters param;
  param.postprocess_only_left = false; 
  Elas elas(param);
  elas.process(I1->data,I2->data,D1_data,D2_data,dims);

  // find maximum disparity for scaling output disparity images to [0..255]
  float disp_max = 0;
  for (int32_t i=0; i<width*height; i++) {
    if (D1_data[i]>disp_max) disp_max = D1_data[i];
    if (D2_data[i]>disp_max) disp_max = D2_data[i];
  }

  // copy float to uchar
  image<uchar> *D1 = new image<uchar>(width,height);
  image<uchar> *D2 = new image<uchar>(width,height);
  for (int32_t i=0; i<width*height; i++) {
    D1->data[i] = (uint8_t)max(255.0*D1_data[i]/disp_max,0.0);
    D2->data[i] = (uint8_t)max(255.0*D2_data[i]/disp_max,0.0);
  }

  // save disparity images
  char output_1[1024];
  char output_2[1024];
  strncpy(output_1,file_1,strlen(file_1)-4);
  strncpy(output_2,file_2,strlen(file_2)-4);
  output_1[strlen(file_1)-4] = '\0';
  output_2[strlen(file_2)-4] = '\0';
  strcat(output_1,"_disp.pgm");
  strcat(output_2,"_disp.pgm");
  savePGM(D1,output_1);
  savePGM(D2,output_2);

  // free memory
  delete I1;
  delete I2;
  delete D1;
  delete D2;
  free(D1_data);
  free(D2_data);
}

cv::Mat DenseStereo::rotateImage(const cv::Mat& source, double angle){
	cv::Point2f src_center(source.cols/2.0, source.rows/2.0);
	cv::Mat rot_mat = cv::getRotationMatrix2D(src_center, angle, 1.0);
	cv::Mat dst;
	cv::warpAffine(source, dst, rot_mat, source.size());
	return dst;
}