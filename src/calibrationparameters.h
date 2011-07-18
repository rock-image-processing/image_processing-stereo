#ifndef CALIBRATIONPARAMETERS_H_
#define CALIBRATIONPARAMETERS_H_

#include <stddef.h>
#include <opencv/cv.h>
#include "dense_stereo_types.h"


namespace dense_stereo {

//TODO do correct error handling
#define ERR_OK				0
#define ERR_NO_VALID_IMAGE 		1
#define ERR_UNSPEC 			2
    
class CalibrationParameters {
public:
	CalibrationParameters();
	virtual ~CalibrationParameters();

	/** load precalculated parameters */
	void loadParameters();
	/// load precalculated parameters from file
	void loadCalibrationFromFile(cv::FileNode &calibration);
	/// save parameters to file
	void saveConfigurationFile(const std::string &filename);
	/** calculates the undistortion parameters and the rectify maps */
	void calculateUndistortAndRectifyMaps();
	/** sets the calibration parameters from CameraCalibration structure
	 * @param camCal camera calibration parameters for stereo camera setup
	 */
	void setStereoCalibrationParameters(const StereoCameraCalibration &stereoCamCal);
	
	/** getter functions */
	//TODO check if they are used
	/*double getTx(){return tx;};
	cv::Mat getQ(){return Q;};
	double getFX1(){return fx1;};*/

	cv::Mat map11, map21, map12, map22;
	int imgWidth, imgHeight;
	cv::Rect roi1, roi2;

private:
	/** different calibration parameters */
	double fx1, fy1, cx1, cy1, d01, d11, d21, d31;
	double fx2, fy2, cx2, cy2, d02, d12, d22, d32;
	double tx, ty, tz;
	double rx, ry, rz;
	cv::Mat cameraMatrix1, cameraMatrix2;
	cv::Mat distCoeffs1, distCoeffs2;
	cv::Mat R, T, Q;

};
}

#endif /* CALIBRATIONPARAMETERS_H_ */
