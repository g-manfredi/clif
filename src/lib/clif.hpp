#ifndef _CLIF_H
#define _CLIF_H

#include <vector>
#include <iostream>

#include "core.hpp"
#include "attribute.hpp"
#include "stringtree.hpp"
#include "datastore.hpp"
#include "dataset.hpp"

#include <opencv2/core/core.hpp>

namespace clif {
  
//std::string path_element(boost::filesystem::path path, int idx);
  
  /*int parse_string_enum(std::string &str, const char **enumstrs);
  int parse_string_enum(const char *str, const char **enumstrs);*/
  
#define DEMOSAIC  1
#define CVT_8U  2
#define UNDISTORT 4
#define CVT_GRAY 8
#define PROCESS_FLAGS_MAX 16


  
  H5::PredType H5PredType(DataType type);
  
  std::vector<std::string> Datasets(H5::H5File &f);
}

class ClifFile
{
public:
  ClifFile() {};
  //TODO create file if not existing?
  ClifFile(std::string &filename, unsigned int flags = H5F_ACC_RDONLY);
  
  void open(std::string &filename, unsigned int flags = H5F_ACC_RDONLY);
  void create(std::string &filename);
  //void close();
  
  clif::Dataset* openDataset(int idx);
  clif::Dataset* openDataset(std::string name);

  clif::Dataset* createDataset(std::string name);
  
  int datasetCount();
  
  bool valid();
  
  const std::vector<std::string> & datasetList() const {return datasets;};
  
  H5::H5File f;
private:
  
  std::vector<std::string> datasets;
};

//TODO here start public cv stuff -> move to extra header files

//only adds methods
//TODO use clif namespace! (?)
namespace clif_cv {
  
  using namespace clif;
  
  DataType CvDepth2DataType(int cv_type);
  int DataType2CvDepth(DataType t);

  cv::Size imgSize(Datastore *store);
    
  void writeCvMat(Datastore *store, uint idx, cv::Mat &m);
  void readCvMat(Datastore *store, uint idx, cv::Mat &m, int flags = 0, float scale = 1.0);
  
  void readCalibPoints(Dataset *set, std::string calib_set_name, std::vector<std::vector<cv::Point2f>> &imgpoints, std::vector<std::vector<cv::Point2f>> &worldpoints);
  void writeCalibPoints(Dataset *set, std::string calib_set_name, std::vector<std::vector<cv::Point2f>> &imgpoints, std::vector<std::vector<cv::Point2f>> &worldpoints);
  
  //void readEPI(ClifDataset *lf, cv::Mat &m, int line, double depth = 0, int flags = 0);

  void writeCvMat(Datastore *store, uint idx, hsize_t w, hsize_t h, void *data);
}

#endif