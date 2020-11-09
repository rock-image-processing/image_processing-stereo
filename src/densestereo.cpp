#include "densestereo.h"
#include "configuration.h"
#include <stdexcept>
#include <opencv2/opencv.hpp>

using namespace std;

namespace stereo {

// wrapper class to hide libelas from orocos
DenseStereo::DenseStereo() 
    : gaussian_kernel(0), calibrationInitialized( false )
{
  // configure Elas and instantiate it
  Elas::parameters elasParam;
  libElasConfiguration config;
  copyToElas( &config, &elasParam );
  elas = new Elas(elasParam);
}

DenseStereo::~DenseStereo() {
  delete elas;
}

//set stereo calibration
void DenseStereo::setStereoCalibration(const frame_helper::StereoCalibration& stereoCal, const int imgWidth, const int imgHeight){
  calParam.setCalibration(stereoCal);
  calParam.setImageSize(cv::Size(imgWidth, imgHeight));
  calParam.initCv();
  
  calibrationInitialized = true;
}

//load libelas parameters (if other then default)
void DenseStereo::setLibElasConfiguration(const libElasConfiguration &libElasParam){
  //delete default configured libelas
  delete elas;

  //configure Elas and instantiate it
  Elas::parameters elasParam;
  copyToElas( &libElasParam, &elasParam );

  elas = new Elas(elasParam);
}

// undistorts and rectifies images with opencv
void DenseStereo::undistortAndRectify(cv::Mat &image, const frame_helper::CameraCalibrationCv& calib){
  cv::Mat newImage;
  
  // undistort/rectify image
  cv::remap(image, newImage, calib.map1, calib.map2, cv::INTER_CUBIC);
  
  image = newImage;
}

// converts an image to grayscale (uint8_t)
void DenseStereo::cvtCvMatToGrayscaleImage(cv::Mat &image) {
  //TODO: Use FrameHelper to avoid double code
  cv::Mat newImage;
  // convert to grayscale image
  switch(image.type()){
    case CV_8UC1:
      //do nothing as the image is already grayscale
      newImage = image;
      break;
    case CV_16UC1:
      image.convertTo(newImage, CV_8U, 1/256.);
      break;
    case CV_8UC3:
      cvtColor( image, newImage, cv::COLOR_BGR2GRAY );
      break;
    case CV_16UC3:
      image.convertTo(newImage, CV_8U, 1/256.);
      cvtColor( image, newImage, cv::COLOR_BGR2GRAY );
      break;
    default:
      throw std::runtime_error("Unknown format. Cannot convert cv::Mat to grayscale.");
  }
  
  if( gaussian_kernel > 0 )
  {
    cv::GaussianBlur( newImage, image, cv::Size( gaussian_kernel, gaussian_kernel ), 0 );
  }
  else
  {
    image = newImage;
  }
}

// computes disparities of image input pair left_frame, right_frame
void DenseStereo::processFramePair (const cv::Mat &left_frame,
                                     const cv::Mat &right_frame,
				     cv::Mat &left_output_frame,
                                     cv::Mat &right_output_frame,
				     bool isRectified )
{
  if (!calibrationInitialized) {
      throw std::runtime_error("Call setStereoCalibration() first!");
  }
  
  // rectify and convert images to Grayscale (uint8_t)
  cv::Mat left = left_frame;
  cv::Mat right = right_frame;
  if( !isRectified )
  {
      undistortAndRectify(left, calParam.camLeft);
      undistortAndRectify(right, calParam.camRight);
  }
  cvtCvMatToGrayscaleImage(left);
  cvtCvMatToGrayscaleImage(right);
  
  // check for correct size
  if (left.size().width <=0 || left.size().height <=0 ||
      right.size().width <=0 || right.size().height <=0 ||
      left.size().width != right.size().width ||
      left.size().height != right.size().height)
  {
    cerr << "ERROR: Images must be of same size, but" << endl;
    cerr << "       left: " << left.size().width << " x " << left.size().height;
    cerr << ", right: " << right.size().width << " x " << right.size().height;
    cerr << endl;

    throw std::runtime_error("Images must be of same size.");
    return;
  }

  // get image width and height
  const int32_t width  = left_frame.size().width;
  const int32_t height = left_frame.size().height;

  // set processing dimensions
  const int32_t dims[3] = {width,height,width}; // bytes per line = width
  // allocate memory for disparity images if not already done
  if (!left_output_frame.data) {
    left_output_frame = cv::Mat(left_frame.size().height,
                                left_frame.size().width,
                                cv::DataType<float>::type);
  }
  if (!right_output_frame.data) {
    right_output_frame = cv::Mat(right_frame.size().height,
                                 right_frame.size().width,
                                 cv::DataType<float>::type);
 }
  
  // process
  elas->process(left.ptr<uint8_t>(),
                right.ptr<uint8_t>(),
                left_output_frame.ptr<float>(),
                right_output_frame.ptr<float>(),
                dims);
}

void disparityToDistance( cv::Mat &disp, float dist_factor )
{
    cv::MatIterator_<float> it = disp.begin<float>(), it_end = disp.end<float>();
    for(; it != it_end; ++it)
    {
	const float disparity = *it;
	*it = disparity > 0 ? 
	    dist_factor / disparity : 
	    std::numeric_limits<float>::quiet_NaN();

    }
}

void DenseStereo::getDistanceImages( cv::Mat &left_disp_image, cv::Mat &right_disp_image )
{
    const frame_helper::StereoCalibration &calib( calParam.getCalibration() );

    // perform conversion to distance image
    disparityToDistance( left_disp_image,
	    fabs( calib.camLeft.fx * calib.extrinsic.tx ) );
    
    disparityToDistance( right_disp_image,
	    fabs( calib.camRight.fx * calib.extrinsic.tx ) );
}

void DenseStereo::getDistanceImages( const cv::Mat &left_frame, const cv::Mat &right_frame,
	cv::Mat &left_output_frame, cv::Mat &right_output_frame,
	bool isRectified )
{
    processFramePair( left_frame, right_frame, left_output_frame, right_output_frame, isRectified );
    getDistanceImages( left_output_frame, right_output_frame );
}

void DenseStereo::getDistanceImages( const cv::Mat &left_frame, const cv::Mat &right_frame,
	base::samples::DistanceImage &left_output_frame, base::samples::DistanceImage &right_output_frame,
	bool isRectified )
{
    cv::Mat 
	cleft = createLeftDistanceImage( left_output_frame ),
	cright = createRightDistanceImage( right_output_frame );

    getDistanceImages( left_frame, right_frame, cleft, cright, isRectified );
}

cv::Mat DenseStereo::createDistanceImage( 
	frame_helper::CameraCalibrationCv const& calibcv, base::samples::DistanceImage& distanceFrame )
{
    const size_t 
	width = calibcv.getImageSize().width, 
	height = calibcv.getImageSize().height, 
	size = width * height;

    // pre-allocate the memory for the output disparity map, so we don't
    // have to copy it. This means we have to assert that the width and
    // height is the same for input and resulting disparity images
    distanceFrame.data.resize( size );	
    distanceFrame.height = height;
    distanceFrame.width = width;	

    const frame_helper::CameraCalibration &calib( calibcv.getCalibration() );

    // scale and center parameters of the distance image are the inverse of the
    // f and c values from the camera calibration. 
    //
    // so analogous for x and y we get scale = 1/f and offset = -c/f
    // 
    distanceFrame.scale_x = 1.0f / calib.fx;
    distanceFrame.scale_y = 1.0f / calib.fy;
    distanceFrame.center_x = -calib.cx / calib.fx; 
    distanceFrame.center_y = -calib.cy / calib.fy; 

    // wrap the data member of the distanceImage into a cv::Mat
    cv::Mat result( height, width, cv::DataType<float>::type,
	    reinterpret_cast<uint8_t *>( &distanceFrame.data[0] ) );

    return result;
}

}
