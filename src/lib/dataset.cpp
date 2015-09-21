#include "dataset.hpp"

#include <opencv2/imgproc/imgproc.hpp>

namespace clif {
void Intrinsics::load(Attributes *attrs, boost::filesystem::path path)
{
  Attribute *a = attrs->getAttribute(path / "type");
  
  _undist_map = cv::Mat();
  
  if (!a) {
    printf("no valid intrinsic model! %s\n", path.c_str());
    model = DistModel::INVALID;
    return;
  }
  
  a->readEnum(model);
  
  f[0] = 0;
  f[1] = 0;
  c[0] = 0;
  c[1] = 0;
  cv_cam = cv::Mat::eye(3,3,CV_64F);
  cv_dist.resize(0);
  
  attrs->getAttribute(path / "projection", f, 2);
  cv_cam.at<double>(0,0) = f[0];
  cv_cam.at<double>(1,1) = f[1];
  
  a = attrs->getAttribute(path / "projection_center");
  if (a) {
    a->get(c, 2);
    cv_cam.at<double>(0,2) = c[0];
    cv_cam.at<double>(1,2) = c[1];
  }
  
  if (model == DistModel::CV8)
    attrs->getAttribute(path / "opencv_distortion", cv_dist);
}

cv::Mat* Intrinsics::getUndistMap(double depth, int w, int h)
{
  if (_undist_map.total())
    return &_undist_map;
  
  cv::Mat tmp;
  
  cv::initUndistortRectifyMap(cv_cam, cv_dist, cv::noArray(), cv::noArray(), cv::Size(w, h), CV_32FC2, _undist_map, tmp);
  
  return &_undist_map;
}

void Dataset::open(H5::H5File &f_, std::string name)
{
  _path = std::string("/clif/").append(name);
  f = f_;
    
  //static_cast<clif::Dataset&>(*this) = clif::Dataset(f, fullpath);
  
  if (h5_obj_exists(f, _path.c_str())) {
    //static_cast<Attributes&>(*this) = Attributes(f, _path);
    Attributes::open(f, _path);
    
    //FIXME specificy which one!?
    load_intrinsics();
  }
  
  if (!Dataset::valid()) {
    printf("could not open dataset %s\n", _path.c_str());
    return;
  }
  
  Datastore::open(this, "data");
}

//TODO use priority!
boost::filesystem::path Dataset::subGroupPath(boost::filesystem::path parent, std::string child)
{
  std::vector<std::string> list;
  
  if (child.size())
    return parent / child;

  listSubGroups(parent, list);
  assert(list.size());
  
  return parent / list[0];
}

Datastore *Dataset::createCalibStore()
{
  if (calib_images)
    return calib_images;
  
  getCalibStore();
  
  if (calib_images)
    return calib_images;
    
  calib_images = new clif::Datastore();
  calib_images->create("calibration/images/data", this);
  return calib_images;
}

//return pointer to the calib image datastore - may be manipulated
Datastore *Dataset::getCalibStore()
{
  boost::filesystem::path dataset_path;
  dataset_path = path() / "calibration/images/data";
  
  std::cout << dataset_path << clif::h5_obj_exists(f, dataset_path) << calib_images << std::endl;
  
  if (!calib_images && clif::h5_obj_exists(f, dataset_path)) {
    calib_images = new clif::Datastore();
    calib_images->open(this, "calibration/images/data");
  }
  
  return calib_images;
}

void Dataset::create(H5::H5File &f_, std::string name)
{
  _path = std::string("/clif/").append(name);
  f = f_;
  
  //TODO check if already exists and fail if it does?
  
  Datastore::create("data", this);
}

boost::filesystem::path Dataset::path()
{
  return boost::filesystem::path(_path);
}

//FIXME
bool Dataset::valid()
{
  return true;
}

void Dataset::load_intrinsics(std::string intrset)
{
  std::vector<std::string> sets;
  
  if (!intrset.size()) {
    listSubGroups("calibration/intrinsics", sets);
    if (!sets.size())
      return;
    //TODO select with priority?!
    intrset = sets[0];
  }
  
  intrinsics.load(this, boost::filesystem::path() / "calibration/intrinsics" / intrset);
}
}