#include "clif_cv.hpp"

#include "dataset.hpp"
#include "preproc.hpp"

#include <sstream>
#include <string>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

namespace clif {
  
typedef unsigned int uint;

  cv::Size imgSize(Datastore *store)
  {
    if (store->dims() < 2)
      return cv::Size(0, 0);
    else
      return cv::Size(store->extent()[0],store->extent()[1]);
  }
    
  //FIXME only power to scales at the moment
  //FIXME store file and dataset name in cache!
  /*void readCvMat(Datastore *store, uint idx, cv::Mat &outm, int flags, float scale)
  {
    if (flags & UNDISTORT) {
      flags |= DEMOSAIC;
    }
    
    cv::Mat *m = static_cast<cv::Mat*>(NULL);
    if (m) {
      outm = *m;
      return;
    }
    
    std::ostringstream longkey_stream;
    std::string longkey;
    
    bool check_cache = true;
    char *xdg_cache = getenv("XDG_CACHE_HOME");
    path cache_path;
    
    if (flags & NO_DISK_CACHE)
      check_cache = false;
    else {
      if (xdg_cache)
        cache_path = xdg_cache;
      else {
        char *home = getenv("HOME");
        if (home) {
          cache_path = home;
          cache_path /= ".cache";
        }
        else
          check_cache = false;
      }
    }
    
    printf("load idx %d\n", idx);
    
    if (check_cache) {
      
      
    std::ostringstream shortkey_stream;
    std::string shortkey;
    
    shortkey_stream << " " << idx << " " << flags << " " << scale;
    shortkey = shortkey_stream.str();
      
      //printf("cache dir %s\n", cache_path.c_str());
      cache_path /= "clif/v0.0/cached_imgs";
      std::hash<std::string> hasher;
      std::string dset_path = store->getDataset()->path().generic_string();
      longkey_stream << hasher(dset_path) << "_" << hasher(store->getDatastorePath()) << "_" << hasher(shortkey);
      longkey = longkey_stream.str();
      cache_path /= longkey+".pgm";

      
      //std::cout << "check " << cache_path << std::endl;
      
      if (boost::filesystem::exists(cache_path)) {
        //printf("cache file!\n");
        m = new cv::Mat();
        *m = cv::imread(cache_path.string(), CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
        outm = *m;
        //store->cache_set(idx,flags,scale, m);
        return;
      }
    }
    
    if (store->org() == DataOrg::BAYER_2x2) {
      //FIXME bayer only for now!
      m = new cv::Mat(imgSize(store), BaseType2CvDepth(store->type()));
      
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
          default :
            abort();
        }
      }
    }
    else if (store->org() == DataOrg::INTERLEAVED && store->order() == DataOrder::RGB) {
      m = new cv::Mat(imgSize(store), CV_MAKETYPE(BaseType2CvDepth(store->type()), 3));
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
      //FIXMe getundistmap (should be) generic!
      if (i->model == DistModel::CV8) {
        cv::Mat newm;
        cv::Mat *map = i->getUndistMap(0, imgSize(store).width, imgSize(store).height);
        //cv::undistort(*m,newm, i->cv_cam, i->cv_dist);
        //cv::setNumThreads(0);
        remap(*m, newm, *map, cv::noArray(), cv::INTER_LINEAR);
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
    
    //store->cache_set(idx,flags,scale, m);
    
    if (check_cache) {
      create_directories(cache_path.parent_path());
      cv::imwrite(cache_path.string(), *m);
    }
    
    outm = *m;
  }*/
  
  static inline int proc_ch_count(int channels, int flags)
  {
    if (flags & Improc::CVT_GRAY)
      return 1;
    return channels;
  }
  
  static void init_planar_mats(Datastore *store, std::vector<cv::Mat> *channels)
  {
    for(uint i=0;i<channels->size();i++)
      (*channels)[i].create(imgSize(store), BaseType2CvDepth(store->type()));
  }
  
  static std::vector<cv::Mat> *new_planar_mats(Datastore *store, int channels)
  {
    std::vector<cv::Mat> *mats = new std::vector<cv::Mat>(channels);
    init_planar_mats(store, mats);
    
    return mats;
  }
  
  //FIXME delete cache on endianess change!
  /*void readMatV(std::vector<cv::Mat> *channels, const char *path)
  {
    uint size = (*channels)[0].elemSize() * (*channels)[0].total();
    FILE *f = fopen(path, "r");
    for(uint c=0;c<channels->size();c++)
      if (size != fread((*channels)[c].data, 1, size, f))
        abort();
  }
  
  //FIXME delete cache on endianess change!
  void writeMatV(std::vector<cv::Mat> *channels, const char *path)
  {
    uint size = (*channels)[0].elemSize() * (*channels)[0].total();
    FILE *f = fopen(path, "w");
    for(uint c=0;c<channels->size();c++)
      if (size != fwrite((*channels)[c].data, 1, size, f))
        abort();
  }*/
  
  //planar version
  /*void readCvMat(Datastore *store, uint idx, std::vector<cv::Mat> &outm, int flags, float scale)
  {
    cv::Size size = imgSize(store);
    int ch_input = store->imgChannels();
    int ch_count = proc_ch_count(ch_input, flags);
    
    if (flags & UNDISTORT) {
      flags |= DEMOSAIC;
    }
    
    //std::ostringstream shortkey_stream;
    //std::string shortkey;
    
    //shortkey_stream << "_" << idx << "_" << flags << " " << scale;
    //shortkey = shortkey_stream.str();
    
    std::vector<cv::Mat> *m = static_cast<std::vector<cv::Mat>*>(NULL);
    if (m) {
      outm = *m;
      return;
    }
    
    std::ostringstream longkey_stream;
    std::string longkey;
    
    bool check_cache = true;
    char *xdg_cache;
    path cache_path;
    
    if (flags & NO_DISK_CACHE)
      check_cache = false;
    else {
      xdg_cache = getenv("XDG_CACHE_HOME");
      
      if (xdg_cache)
        cache_path = xdg_cache;
      else {
        char *home = getenv("HOME");
        if (home) {
          cache_path = home;
          cache_path /= ".cache";
        }
        else
          check_cache = false;
      }
    }
    
    printf("load idx %d\n", idx);
    
    if (check_cache) {
          std::ostringstream shortkey_stream;
    std::string shortkey;
    
    shortkey_stream << "_" << idx << "_" << flags << " " << scale;
    shortkey = shortkey_stream.str();
      
      //printf("cache dir %s\n", cache_path.c_str());
      cache_path /= "clif/v0.0/cached_imgs";
      std::hash<std::string> hasher;
      std::string dset_path = store->getDataset()->path().generic_string();
      longkey_stream << hasher(dset_path) << "_" << hasher(store->getDatastorePath()) << "_" << hasher(shortkey);
      longkey = longkey_stream.str();
      cache_path /= longkey+".raw";
      
      if (boost::filesystem::exists(cache_path.string().c_str())) {
        m = new_planar_mats(store, ch_count);
        readMatV(m, cache_path.string().c_str());
        outm = *m;
        //store->cache_set(idx,flags,scale, m);
        return;
      }
    }
    
    if (store->org() == DataOrg::BAYER_2x2) {
      m = new_planar_mats(store, ch_count);
      cv::Mat tmp(size, BaseType2CvDepth(store->type()));
      
      store->readRawImage(idx, tmp.size().width, tmp.size().height, tmp.data);
      
      if (store->org() == DataOrg::BAYER_2x2 && flags & DEMOSAIC) {
        switch (store->order()) {
          case DataOrder::RGGB :
            cvtColor(tmp, tmp, CV_BayerBG2BGR);
            break;
          case DataOrder::BGGR :
            cvtColor(tmp, tmp, CV_BayerRG2BGR);
            break;
          case DataOrder::GBRG :
            cvtColor(tmp, tmp, CV_BayerGR2BGR);
            break;
          case DataOrder::GRBG :
            cvtColor(tmp, tmp, CV_BayerGB2BGR);
            break;
          default :
            abort();
        }
        cv::split(tmp,*m);
      }
    }
    else if (store->org() == DataOrg::INTERLEAVED && store->order() == DataOrder::RGB) {
      m = new_planar_mats(store, ch_count);
      cv::Mat tmp (size, CV_MAKETYPE(BaseType2CvDepth(store->type()), 3));
      store->readRawImage(idx, tmp.size().width, tmp.size().height, tmp.data);
      cv::split(tmp,*m);
    }
    //FIXME READ PLANAR!
    
#pragma omp parallel for schedule(dynamic)
    for(uint c=0;c<m->size();c++) {
      cv::Mat *ch = &(*m)[c];
    
      if (ch->depth() == CV_16U && flags & CVT_8U) {
        *ch *= 1.0/256.0;
        ch->convertTo(*ch, CV_8U);
      }
      
      //TODO FIXME!
      if (ch->channels() != 1 && flags & CVT_GRAY) {
        abort();
        cvtColor(*ch, *ch, CV_BGR2GRAY);
      }
      
      if (flags & UNDISTORT) {
        Intrinsics *i = &store->getDataset()->intrinsics;
        //FIXMe getundistmap (should be) generic!
        if (i->model == DistModel::CV8) {
          cv::Mat newm;
          cv::Mat *chap = i->getUndistMap(0, size.width, size.height);
          //cv::undistort(*ch,newm, i->cv_cam, i->cv_dist);
          //cv::setNumThreads(0);
          remap(*ch, newm, *chap, cv::noArray(), cv::INTER_LINEAR);
          *ch = newm;
        }
        else
          printf("distortion model not supported: %s\n", enum_to_string(i->model));
      }
      
      if (scale != 1.0) {
        int iscale = 1/scale;
        cv::GaussianBlur(*m,*m,cv::Size(iscale*2+1,iscale*2+1), 0);
        cv::resize(*m,*m,cv::Point2i(m->size())*scale, cv::INTER_NEAREST);
      }
    }
    
    //store->cache_set(idx,flags,scale, m);
    
    if (check_cache) {
      create_directories(cache_path.parent_path());
      writeMatV(m, cache_path.string().c_str());
    }
    
    outm = *m;
  }*/
  
  
static cv::Mat mat_2d_from_3d(const cv::Mat *m, int ch)
{
  //step is already in bytes!
  return cv::Mat(m->size[1], m->size[2], m->depth(), m->data + ch*m->step[0]);
}
  
void clifMat2cv(cv::Mat *in, cv::Mat *out)
{
  std::vector<cv::Mat> channels(in->size[0]);

  for(int i=0;i<in->size[0];i++)
    //FIXME need clone else merge will memcpy overlaying memory areas. why?
    channels[i] = mat_2d_from_3d(in, i);

  out->create(in->size[1], in->size[2], in->type());
  cv::merge(channels, *out);
}

void cv2ClifMat(cv::Mat *in, cv::Mat *out)
{
  std::vector<cv::Mat> channels;
  
  cv::split(*in, channels);
  
  int imgsize[3];
  imgsize[2] = in->size().width;
  imgsize[1] = in->size().height;
  imgsize[0] = channels.size();
  
  out->create(3,imgsize, in->depth());

  for(int i=0;i<channels.size();i++) {
    cv::Mat channel = mat_2d_from_3d(out, i);
    channels[i].copyTo(channel);
  }
}

int clifMat_channels(cv::Mat &img)
{
  assert(img.dims == 3);
  return img.size[0];
}

cv::Mat clifMat_channel(cv::Mat &m, int ch)
{
  assert(m.dims == 3);
  return cv::Mat(m.size[1], m.size[2], m.depth(), m.data + ch*m.step[0]);
}
  
}