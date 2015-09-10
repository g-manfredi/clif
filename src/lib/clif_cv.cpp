#include "clif_cv.hpp"

#include <opencv2/imgproc/imgproc.hpp>

namespace clif {
  
  cv::Size imgSize(Datastore *store)
  {
    H5::DataSpace space = store->H5DataSet().getSpace();
    hsize_t dims[3];
    hsize_t maxdims[3];
    
    space.getSimpleExtentDims(dims, maxdims);
    
    dims[0] /= combinedTypeElementCount(store->type(), store->org(), store->order());
    dims[1] /= combinedTypePlaneCount(store->type(), store->org(), store->order());
    
    return cv::Size(dims[0],dims[1]);
  }
  
  //TODO create mappings with cmake?
  DataType CvDepth2DataType(int cv_type)
  {
    switch (cv_type) {
      case CV_8U : return clif::DataType::UINT8;
      case CV_16U : return clif::DataType::UINT16;
      default :
        abort();
    }
  }
  
  int DataType2CvDepth(DataType t)
  {
    switch (t) {
      case clif::DataType::UINT8 : return CV_8U;
      case clif::DataType::UINT16 : return CV_16U;
      default :
        abort();
    }
  }
    
  void readCvMat(Datastore *store, uint idx, cv::Mat &outm, int flags, float scale)
  {
    if (flags & UNDISTORT) {
      flags |= DEMOSAIC;
    }
    //FIXME scale is a BAAAAAD hack!
    uint64_t key = idx*PROCESS_FLAGS_MAX | flags | (((uint64_t)((uint32_t*)&scale)) << 32);
    
    cv::Mat *m = static_cast<cv::Mat*>(store->cache_get(key));
    if (m) {
      outm = *m;
      return;
    }
    
    if (store->org() == DataOrg::BAYER_2x2) {
      //FIXME bayer only for now!
      m = new cv::Mat(imgSize(store), DataType2CvDepth(store->type()));
      
      store->readRawImage(idx, m->size().width, m->size().height, m->data);
      
      if (store->org() == DataOrg::BAYER_2x2 && flags & DEMOSAIC) {
        switch (store->order()) {
          case DataOrder::RGGB :
            cvtColor(*m, *m, CV_BayerBG2BGR);
            break;
          case DataOrder::BGGR :
            cvtColor(*m, *m, CV_BayerRG2BGR);
            break;
          case DataOrder::GBRG :
            cvtColor(*m, *m, CV_BayerGR2BGR);
            break;
          case DataOrder::GRBG :
            cvtColor(*m, *m, CV_BayerGB2BGR);
            break;
        }
      }
    }
    else if (store->org() == DataOrg::INTERLEAVED && store->order() == DataOrder::RGB) {
      m = new cv::Mat(imgSize(store), CV_MAKETYPE(DataType2CvDepth(store->type()), 3));
      store->readRawImage(idx, m->size().width, m->size().height, m->data);
    }
    
    if (m->depth() == CV_16U && flags & CVT_8U) {
      *m *= 1.0/256.0;
      m->convertTo(*m, CV_8U);
    }
    
    if (m->channels() != 1 && flags & CVT_GRAY) {
      cvtColor(*m, *m, CV_BGR2GRAY);
    }
    
    if (flags & UNDISTORT) {
      Intrinsics *i = &store->getDataset()->intrinsics;
      if (i->model == DistModel::CV8) {
        cv::Mat newm;
        cv::undistort(*m,newm, i->cv_cam, i->cv_dist);
        *m = newm;
      }
      else
        printf("distortion model not supported: %s\n", enum_to_string(i->model));
    }
    
    if (scale != 1.0) {
      int iscale = 1/scale;
      cv::GaussianBlur(*m,*m,cv::Size(iscale*2+1,iscale*2+1), 0);
      cv::resize(*m,*m,cv::Point2i(m->size())*scale, cv::INTER_NEAREST);
    }
    
    store->cache_set(key, m);
    
    outm = *m;
  }
  
  
  void writeCalibPoints(Dataset *set, std::string calib_set_name, std::vector<std::vector<cv::Point2f>> &imgpoints, std::vector<std::vector<cv::Point2f>> &worldpoints)
  {
    int pointcount = 0;
    
    for(int i=0;i<imgpoints.size();i++)
      for(int j=0;j<imgpoints[i].size();j++)
        pointcount++;
      
    float *pointbuf = new float[4*pointcount];
    float *curpoint = pointbuf;
    int *sizebuf = new int[imgpoints.size()];
    
    
    for(int i=0;i<imgpoints.size();i++) {
      sizebuf[i] = imgpoints[i].size();
      for(int j=0;j<imgpoints[i].size();j++) {
        curpoint[0] = imgpoints[i][j].x;
        curpoint[1] = imgpoints[i][j].y;
        curpoint[2] = worldpoints[i][j].x;
        curpoint[3] = worldpoints[i][j].y;
        curpoint += 4;
      }
    }
      
    set->setAttribute(boost::filesystem::path() / "calibration/images/sets" / calib_set_name / "pointdata", pointbuf, 4*pointcount);
    set->setAttribute(boost::filesystem::path() / "calibration/images/sets" / calib_set_name / "pointcounts", sizebuf, imgpoints.size());
  }
  
  void readCalibPoints(Dataset *set, std::string calib_set_name, std::vector<std::vector<cv::Point2f>> &imgpoints, std::vector<std::vector<cv::Point2f>> &worldpoints)
  {
    std::vector<float> pointbuf;
    std::vector<int> sizebuf;
    
    set->getAttribute(boost::filesystem::path() / "calibration/images/sets" / calib_set_name / "pointdata", pointbuf);  
    set->getAttribute(boost::filesystem::path() / "calibration/images/sets" / calib_set_name / "pointcounts", sizebuf);
    
    imgpoints.clear();
    imgpoints.resize(sizebuf.size());
    worldpoints.clear();
    worldpoints.resize(sizebuf.size());

    int idx = 0;
    for(int i=0;i<sizebuf.size();i++) {
      imgpoints[i].resize(sizebuf[i]);
      worldpoints[i].resize(sizebuf[i]);
      for(int j=0;j<sizebuf[i];j++) {
        imgpoints[i][j] = cv::Point2f(pointbuf[idx+0],pointbuf[idx+1]);
        worldpoints[i][j] = cv::Point2f(pointbuf[idx+2],pointbuf[idx+3]);
        idx += 4;
      }
    }
  }
}