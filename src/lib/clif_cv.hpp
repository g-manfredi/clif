#ifndef _CLIF_CV_H
#define _CLIF_CV_H

#include "clif.hpp"

#include <opencv2/core/core.hpp>

namespace clif {
  
  BaseType CvDepth2DataType(int cv_type);
  int DataType2CvDepth(BaseType t);

  cv::Size imgSize(Datastore *store);
    
  void writeCvMat(Datastore *store, uint idx, cv::Mat &m);
  void readCvMat(Datastore *store, uint idx, cv::Mat &m, int flags = 0, float scale = 1.0);
  void readCvMat(Datastore *store, uint idx, std::vector<cv::Mat> &outm, int flags = 0, float scale = 1.0);
  
  void readCalibPoints(Dataset *set, std::string calib_set_name, std::vector<std::vector<cv::Point2f>> &imgpoints, std::vector<std::vector<cv::Point2f>> &worldpoints);
  void writeCalibPoints(Dataset *set, std::string calib_set_name, std::vector<std::vector<cv::Point2f>> &imgpoints, std::vector<std::vector<cv::Point2f>> &worldpoints);
  
  //void readEPI(ClifDataset *lf, cv::Mat &m, int line, double depth = 0, int flags = 0);

  void writeCvMat(Datastore *store, uint idx, hsize_t w, hsize_t h, void *data);
  
}

#endif