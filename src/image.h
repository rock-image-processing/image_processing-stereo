/*
Copyright 2011. All rights reserved.
Institute of Measurement and Control Systems
Karlsruhe Institute of Technology, Germany

This file is part of libelas.
Authors: Andreas Geiger, Jan Frädrich

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

// basic image I/O, based on Pedro Felzenszwalb's code

#ifndef IMAGE_H
#define IMAGE_H

#include <cstdlib>
#include <climits>
#include <cstring>
#include <fstream>

#include <opencv/cv.h>

#include "calibrationparameters.h"
#include "imageprocessing.h"

// use imRef to access image data.
#define imRef(im, x, y) (im->access[y][x])
  
// use imPtr to get pointer to image data.
#define imPtr(im, x, y) &(im->access[y][x])

#define BUF_SIZE 256

typedef unsigned char uchar;
typedef struct { uchar r, g, b; } rgb;

inline bool operator==(const rgb &a, const rgb &b) {
  return ((a.r == b.r) && (a.g == b.g) && (a.b == b.b));
}

// image class
template <class T> class image {
public:

  // create image
  image(const int width, const int height, const bool init = false);

  // delete image
  ~image();

  // init image
  void init(const T &val);

  // deep copy
  image<T> *copy() const;
  
  // get image width/height
  int width() const { return w; }
  int height() const { return h; }
  
  // image data
  T *data;
  
  // row pointers
  T **access;
  
private:
  int w, h;
};

template <class T> image<T>::image(const int width, const int height, const bool init) {
  w = width;
  h = height;
  data = new T[w * h];  // allocate space for image data
  access = new T*[h];   // allocate space for row pointers
  
  // initialize row pointers
  for (int i = 0; i < h; i++)
    access[i] = data + (i * w);  
  
  // init to zero
  if (init)
    memset(data, 0, w * h * sizeof(T));
}

template <class T> image<T>::~image() {
  delete [] data; 
  delete [] access;
}

template <class T> void image<T>::init(const T &val) {
  T *ptr = imPtr(this, 0, 0);
  T *end = imPtr(this, w-1, h-1);
  while (ptr <= end)
    *ptr++ = val;
}


template <class T> image<T> *image<T>::copy() const {
  image<T> *im = new image<T>(w, h, false);
  memcpy(im->data, data, w * h * sizeof(T));
  return im;
}

class pnm_error {};

void pnm_read(std::ifstream &file, char *buf) {
  char doc[BUF_SIZE];
  char c;
  
  file >> c;
  while (c == '#') {
    file.getline(doc, BUF_SIZE);
    file >> c;
  }
  file.putback(c);
  
  file.width(BUF_SIZE);
  file >> buf;
  file.ignore();
}

image<uchar> *loadPGM(const char *name) {
  char buf[BUF_SIZE];
  
  // read header
  std::ifstream file(name, std::ios::in | std::ios::binary);
  pnm_read(file, buf);
  if (strncmp(buf, "P5", 2)) {
    std::cout << "ERROR: Could not read file " << name << std::endl;
    throw pnm_error();
  }

  pnm_read(file, buf);
  int width = atoi(buf);
  pnm_read(file, buf);
  int height = atoi(buf);

  pnm_read(file, buf);
  if (atoi(buf) > UCHAR_MAX) {
    std::cout << "ERROR: Could not read file " << name << std::endl;
    throw pnm_error();
  }

  // read data
  image<uchar> *im = new image<uchar>(width, height);
  file.read((char *)imPtr(im, 0, 0), width * height * sizeof(uchar));

  return im;
}

// converts an image to grayscale and image<uchar>
image<uchar> *cvtCvMatToGrayscaleImage(const cv::Mat &inImage) {
  int imgWidth = inImage.size().width;
  int imgHeight = inImage.size().height;
  cv::Mat Image;
  
  // convert to grayscale image
  switch(inImage.type()){
    case CV_8UC1:
      inImage.copyTo(Image);
      break;
    case CV_16UC1:
      inImage.convertTo(Image, CV_8U, 1/256.);
      break;
    case CV_8UC3:
      cvtColor( inImage, Image, CV_BGR2GRAY );
      break;
    case CV_16UC3:
      inImage.convertTo(Image, CV_8U, 1/256.);
      cvtColor( Image, Image, CV_BGR2GRAY );
      break;
    default:
      std::cerr << "Unknown format. Cannot convert cv::Mat to image<uchar>." << std::endl;
      throw pnm_error();
  }
  
  // create image structure
  image<uchar> *im = new image<uchar>(imgWidth, imgHeight, false);
  
  // copy to structure that libelas can use
  unsigned char * img_data = (unsigned char *)Image.data;
  memcpy((void*)im->data,(void*)img_data,imgWidth*imgHeight);
  
  return im;
}

cv::Mat convertImage2CvMat(image<uchar> *im) {
  int width = im->width();
  int height = im->height();
  cv::Mat Image = cv::Mat(height, width, CV_8UC1, im->data);
  cv::Mat deepCopyImage = Image.clone(); //deep copy to avoid problems with deletion of im
  return deepCopyImage;
}

image<uchar> *loadImage(const char *name, bool right_image) {
  // load images with opencv
  ImageProcessing *imgproc = new ImageProcessing();//for reading and rectifying the image

  //load default calibration parameters
  CalibrationParameters *cp = new CalibrationParameters();
  cp->loadParameters();
  cp->calculateUndistortAndRectifyMaps();
  
  int imgWidth = cp->imgWidth;
  int imgHeight = cp->imgHeight;

  // image in grayscale
  IplImage *curImage = cvCreateImage(cvSize(imgWidth, imgHeight), IPL_DEPTH_8U, 1);
  
  // read image
  int result = imgproc->acquireImage(curImage, name);
  if(result != 0)
    {
      std::cout << "Error obtaining image " << name << std::endl;
      throw pnm_error();
    }
  
  // rectify image
  result = imgproc->preprocessImage(curImage, right_image, cp);
  if(result != 0)
    {
      std::cout << "Error preprocessing image " << name << std::endl;
      throw pnm_error();
    }
  
  image<uchar> *im = cvtCvMatToGrayscaleImage(curImage);

  delete imgproc;
  delete cp;
  cvReleaseImage(&curImage);
  
  return im;
}

void savePGM(image<uchar> *im, const char *name) {
  int width = im->width();
  int height = im->height();
  std::ofstream file(name, std::ios::out | std::ios::binary);

  file << "P5\n" << width << " " << height << "\n" << UCHAR_MAX << "\n";
  file.write((char *)imPtr(im, 0, 0), width * height * sizeof(uchar));
  
  /*if(!strncmp(name,".png",4))
  {
    cvSaveImage(name, curRightImage);
  }*/
  
}

#endif
