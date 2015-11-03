#ifndef _CLIF_CV_H
#define _CLIF_CV_H

#include "clif.hpp"
#include "opencv2/core/core.hpp"

namespace clif {
  
class Dataset;
class Datastore;
  
/** \defgroup clif_cv OpenCV Bindings
*  @{
*/
  cv::Size imgSize(Datastore *store);
    
  //void writeCvMat(Datastore *store, uint idx, cv::Mat &m);
  //void readCvMat(Datastore *store, uint idx, cv::Mat &m, int flags = 0, float scale = 1.0);
  //void readCvMat(Datastore *store, uint idx, std::vector<cv::Mat> &outm, int flags = 0, float scale = 1.0);
  
  void readCalibPoints(Dataset *set, std::string calib_set_name, std::vector<std::vector<cv::Point2f>> &imgpoints, std::vector<std::vector<cv::Point2f>> &worldpoints);
  void writeCalibPoints(Dataset *set, std::string calib_set_name, std::vector<std::vector<cv::Point2f>> &imgpoints, std::vector<std::vector<cv::Point2f>> &worldpoints);
  
  //void readEPI(ClifDataset *lf, cv::Mat &m, int line, double depth = 0, int flags = 0);

  //void writeCvMat(Datastore *store, uint idx, hsize_t w, hsize_t h, void *data);
  
  void clifMat2cv(cv::Mat *in, cv::Mat *out);
  void cv2ClifMat(cv::Mat *in, cv::Mat *out);
  
  int clifMat_channels(cv::Mat &img);
  cv::Mat clifMat_channel(cv::Mat &m, int ch);
  
/**
 *  @}
 */
}

#endif