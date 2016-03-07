#include "preproc.hpp"

#include "mat.hpp"
#include "datastore.hpp"
#include "dataset.hpp"
#include <opencv2/imgproc/imgproc.hpp>

#include "cam.hpp"

namespace clif {
  
ProcData::ProcData(int flags, Datastore *store, double min, double max, double depth, Interpolation interp, float scale, int every)
: _flags(flags), _min(min), _max(max), _depth(depth), _interpolation(interp), _scale(scale), _every(every)
{  
  if (store)
    set_store(store);
}

void ProcData::set_store(Datastore *store)
{
  if (_store == store)
    return;
    
  _store = store;
  
  _w = _store->extent()[0];
  _h = _store->extent()[1];
  _d = _store->extent()[2];
  
  //FIXME use path from datastore!
  try {
    _intrinsics = _store->dataset()->getSubGroup("calibration/intrinsics");
  }
  catch (std::runtime_error) {}
}

Datastore* ProcData::store() const
{
  return _store;
}

void ProcData::set_flags(int flags)
{
  _flags = flags;
}

BaseType ProcData::type()
{
  if (flags() & CVT_8U)
    return BaseType::UINT8;
  
  return _store->type();
}

int ProcData::flags() const
{
  int flags = _flags;
  
  if (flags & CVT_GRAY)
    flags |= DEMOSAIC;
  
  if (flags & UNDISTORT) 
    flags |= DEMOSAIC;
  
  if (_store && (flags & DEMOSAIC) && _store->org() != DataOrg::BAYER_2x2)
    flags &= ~DEMOSAIC;

  if (!std::isnan(_min) || !std::isnan(_max)) {
    flags |= NO_MEM_CACHE;
    flags |= NO_DISK_CACHE;
  }
  
  return flags;
}

int ProcData::w() const
{
  return scale(_w);
}

int ProcData::h() const
{
  return scale(_h);
}

int ProcData::d() const
{
  int channels = _d;
  
  if (flags() & DEMOSAIC)
    channels = 3;
  
  if (flags() & CVT_GRAY)
    channels = 1;
    
  return channels;
}
  
//process from channel(s) to image - input should always be 3d?
//in _MUST_ be 3d with third dim color!

/*  
DEMOSAIC = 1, 
CVT_8U = 2,
UNDISTORT = 4,
CVT_GRAY = 8,
*/

#define CHECK_FLAG_PREPARE(FLAG) if (flags & FLAG) { \
    curr_in = curr_out; \
    if (!(flags ^ FLAG)) { \
      //write to final output \
      curr_out = out; \
    }\
    else { \
      //use new matrix \
      curr_out.release(); }}
      
static bool _handle_preproc(int flag, Mat &curr_in, Mat &curr_out, Mat &out, int &flags)
{
  if (flags & flag) {
    curr_in = curr_out;

    if (!(flags & (~flag)))  
      //write to final output  
      curr_out = out;  
    else  
      //use new matrix  
      curr_out.release();
    
    flags &= ~flag;
  
    return true;
  }
  
  return false;
}
/*
void proc_image(Datastore *store, Mat &in, Mat &out, int flags, const Idx & pos, double min, double max, int scaledown, double depth)
{
  int flags_remain;
  Mat curr_in;
  Mat curr_out = in;
  
  flags &= ~NO_MEM_CACHE;
  flags &= ~NO_DISK_CACHE;
  
  bool sub = false;
  bool scale = false;
  double scale_val;
  
  if (!isnan(min))
    sub = true;
  else
    min = 0.0;
  
  if (!isnan(max)) {
    scale = true;
    scale_val = max-min;
  }
  else {
    scale_val = BaseType_max(in.type())-min;
    if (in.type() == BaseType::UINT32) {
      //UINT32 is converted to float by cvMat :-(
      scale_val = BaseType_max(BaseType::FLOAT)-min;
    }
  }
    
  int cv_interpolation = cv::INTER_LINEAR;
  
  if (flags & HQ) {
    cv_interpolation = cv::INTER_LANCZOS4;
    flags &= ~HQ;
  }
  
  //FIXME hdr may need 4!
  assert(in.size() == 3);
  
  if (_handle_preproc(Improc::DEMOSAIC, curr_in, curr_out, out, flags)) {
    cv::Mat cv_out;
    
    cv::cvtColor(cvMat(curr_in.bind(2,0)), cv_out, order2cv_conf_flag(store->order()));
    
    curr_out = Mat3d(cv_out);
  }
  
  if (flags == 0)
  {
    printf("FIXME implement copy!\n");
  }
  
  if (_handle_preproc(Improc::CVT_GRAY, curr_in, curr_out, out, flags)) {
    cv::Mat accumulate;
    
    //FIXME whats about images in double format?
    cvMat(curr_in.bind(2,0)).convertTo(accumulate, CV_32F);
    for(int c=1;c<curr_in[2];c++)
       cv::add(accumulate, cvMat(curr_in.bind(2,c)), accumulate, cv::noArray(), accumulate.type());
    
    accumulate *= 1.0/curr_in[2];
    
    
    Idx outsize = curr_in;
    outsize[2] = 1;
    curr_out.create(curr_in.type(), outsize);
    accumulate.copyTo(cvMat(curr_out.bind(2,0)));
  }
  
  if (_handle_preproc(Improc::CVT_8U, curr_in, curr_out, out, flags)) {
    curr_out.create(BaseType::UINT8, curr_in);
    
    if (sub) {
      cv::Mat subbed = cvMat(curr_in) - min;
      subbed.convertTo(cvMat(curr_out), CV_8U, BaseType_max<uint8_t>()/scale_val);
    }
    else
      cvMat(curr_in).convertTo(cvMat(curr_out), CV_8U, BaseType_max<uint8_t>()/scale_val);
    
    sub = false;
    scale = false;
  }
  
  if (sub || scale) {
    curr_in = curr_out;

    if (!flags)
      //write to final output  
      curr_out = out;  
    else  
      //use new matrix  
      curr_out.release();
    
    curr_out.create(curr_in.type(), curr_in);
    
    cv::Mat tmp = (cvMat(curr_in)-min) * BaseType_max(curr_in.type())/scale_val;
    tmp.copyTo(cvMat(curr_out));
  }
  
  if (_handle_preproc(Improc::UNDISTORT, curr_in, curr_out, out, flags)) {
    //FIXME path logic? which undist to use?
    DepthDist *undist;
//#pragma omp critical
    {
    undist = store->undist(depth);
    
    if (undist)
      undist->undistort(curr_in, curr_out, pos, cv_interpolation);
    else {
      printf("distortion model not supported\n");
      abort();
      }
    }
  }
  
  out = curr_out;
  
  if (flags) {
    printf("WARNING: proc_image() did not handle all processing flags!\n ");
  }
}*/


void proc_image(Mat &in, Mat &out, const ProcData & proc, const Idx & pos)
{
  int flags_remain;
  Mat curr_in;
  Mat curr_out = in;
  
  int flags = proc.flags();
  double min = proc.min();
  double max = proc.max();
  Datastore *store = proc.store();
  double depth = proc.depth();
    
  flags &= ~NO_MEM_CACHE;
  flags &= ~NO_DISK_CACHE;
  
  //add placeholder for image scaling
  if (proc.scale() != 1.0)
    flags |= Improc::_SCALE;
  
  bool sub = false;
  bool scale = false;
  double scale_val;
  
  if (!std::isnan(min))
    sub = true;
  else
    min = 0.0;
  
  if (!std::isnan(max)) {
    scale = true;
    scale_val = max-min;
  }
  else {
    scale_val = BaseType_max(in.type())-min;
    if (in.type() == BaseType::UINT32) {
      //UINT32 is converted to float by cvMat :-(
      scale_val = BaseType_max(BaseType::FLOAT)-min;
    }
  }
    
  int cv_interpolation = cv::INTER_LINEAR;
  
  if (flags & HQ) {
    cv_interpolation = cv::INTER_LANCZOS4;
    flags &= ~HQ;
  }
  
  //FIXME hdr may need 4!
  assert(in.size() == 3);
  
  if (flags == 0)
  {
    printf("FIXME implement copy!\n");
    out.create(in.type(), in);
    cvMat(in).copyTo(cvMat(out));
    return;
  }
  
  if (_handle_preproc(Improc::DEMOSAIC, curr_in, curr_out, out, flags)) {
    curr_out.create(curr_in.type(), {curr_in.r(0,1),3});
    cv::Mat cv_out;    
    cv::cvtColor(cvMat(curr_in.bind(2,0)), cv_out, order2cv_conf_flag(store->order()));
    
    //FIXME copy!
    //curr_out = Mat3d(cv_out);
    cvMat(Mat3d(cv_out)).copyTo(cvMat(curr_out));
  }
  
  if (_handle_preproc(Improc::CVT_GRAY, curr_in, curr_out, out, flags)) {
    cv::Mat accumulate;
        
    //FIXME whats about images in double format?
    cvMat(curr_in.bind(2,0)).convertTo(accumulate, CV_32F);
    for(int c=1;c<curr_in[2];c++)
       cv::add(accumulate, cvMat(curr_in.bind(2,c)), accumulate, cv::noArray(), accumulate.type());
    
    accumulate *= 1.0/curr_in[2];
    
    
    Idx outsize = curr_in;
    outsize[2] = 1;
    curr_out.create(curr_in.type(), outsize);
    accumulate.copyTo(cvMat(curr_out.bind(2,0)));
  }
  
  if (_handle_preproc(Improc::CVT_8U, curr_in, curr_out, out, flags)) {
    curr_out.create(BaseType::UINT8, curr_in);
    
    if (sub) {
      cv::Mat subbed = cvMat(curr_in) - min;
      subbed.convertTo(cvMat(curr_out), CV_8U, BaseType_max<uint8_t>()/scale_val);
    }
    else
      cvMat(curr_in).convertTo(cvMat(curr_out), CV_8U, BaseType_max<uint8_t>()/scale_val);
    
    sub = false;
    scale = false;
  }
  
  if (sub || scale) {
    curr_in = curr_out;

    if (!flags)
      //write to final output  
      curr_out = out;  
    else  
      //use new matrix  
      curr_out.release();
    
    curr_out.create(curr_in.type(), curr_in);
    
    cv::Mat tmp = (cvMat(curr_in)-min) * BaseType_max(curr_in.type())/scale_val;
    tmp.copyTo(cvMat(curr_out));
  }
  
  if (_handle_preproc(Improc::UNDISTORT, curr_in, curr_out, out, flags)) {
    //FIXME path logic? which undist to use?
    DepthDist *undist;
//#pragma omp critical
    {
    undist = store->undist(depth);
    
    if (undist)
      undist->undistort(curr_in, curr_out, pos, cv_interpolation);
    else {
      printf("distortion model not supported\n");
      abort();
      }
    }
  }
  
  if (_handle_preproc(Improc::_SCALE, curr_in, curr_out, out, flags)) {
    float sigma = 0.5/proc.scale();
    int ks = int(sigma)*6+1;
    cv::Mat tmp;
    
    //FIXME sync size with opencv resize sizing (also in other places!)
    curr_out.create(curr_in.type(), {proc.scale(curr_in[0]), proc.scale(curr_in[1]), curr_in[2]});

    for(int c=0;c<curr_in[2];c++) {
      cv::GaussianBlur(cvMat(curr_in.bind(2,c)), tmp, cv::Size(ks,ks), sigma, sigma);
      cv::resize (tmp, cvMat(curr_out.bind(2,c)), cv::Size(0,0), proc.scale(), proc.scale(), cv::INTER_NEAREST);
    }
  }
   
  out = curr_out;
  
  if (flags) {
    printf("WARNING: proc_image() did not handle all processing flags!\n ");
  }
}

} //namespace clif
