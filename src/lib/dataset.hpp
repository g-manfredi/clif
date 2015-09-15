#ifndef _CLIF_DATASET_H
#define _CLIF_DATASET_H

#include <opencv2/core/core.hpp>

#include "attribute.hpp"
#include "datastore.hpp"

namespace clif {
  
class Datastore;

class Intrinsics {
  public:
    Intrinsics() {};
    Intrinsics(Attributes *attrs, boost::filesystem::path &path) { load(attrs, path); };
    void load(Attributes *attrs, boost::filesystem::path path);
    cv::Mat* getUndistMap(double depth, int w, int h);
    
    double f[2], c[2];
    DistModel model = DistModel::INVALID;
    std::vector<double> cv_dist;
    cv::Mat cv_cam;

  private:
    cv::Mat _undist_map;
};
  
class Dataset : public Attributes, public Datastore {
  public:
    Dataset() {};
    //FIXME use open/create methods
    Dataset(H5::H5File &f_, std::string path);
    
    void open(H5::H5File &f_, std::string name);
    void create(H5::H5File &f_, std::string name);
    
    //void set(Datastore &data_) { data = data_; };
    //TODO should this call writeAttributes (and we completely hide io?)
    void setAttributes(Attributes &attrs) { static_cast<Attributes&>(*this) = attrs; };   
    //writes only Attributes! FIXME hide Attributes::Write
    void writeAttributes() { Attributes::write(f, _path); }
    
    Datastore *getCalibStore();
    Datastore *createCalibStore();
    
    bool valid();
    
    void load_intrinsics(std::string intrset = std::string());
    
    boost::filesystem::path subGroupPath(boost::filesystem::path parent, std::string child = std::string());
    
    boost::filesystem::path path();
    
    H5::H5File f;
    std::string _path;

    //TODO if attributes get changed automatically refresh intrinsics on getIntrinsics?
    Intrinsics intrinsics;
    
    Datastore *calib_images = NULL;
};
  
}

#endif