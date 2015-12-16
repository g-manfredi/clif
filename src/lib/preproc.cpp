#include "preproc.hpp"

#include "mat.hpp"
#include "datastore.hpp"
#include "dataset.hpp"
#include <opencv2/imgproc/imgproc.hpp>

#include "cam.hpp"

namespace clif {
  
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

void proc_image(Datastore *store, Mat &in, Mat &out, int flags, double min, double max, int scaledown, double depth)
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
  else
    scale_val = BaseType_max(in.type())-min;
    
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
    DepthDist *undist = store->undist(depth);
    
    if (undist)
      undist->undistort(curr_in, curr_out, cv_interpolation);
    else {
      /*curr_out.create(curr_in.type(), curr_in);
      
      Intrinsics *i = &store->dataset()->intrinsics;
      //FIXME get undist map (should be) generic!
      if (i->model == DistModel::INVALID) {} // do nothing
      else if (i->model == DistModel::CV8) {
        cv::Mat *chap = i->getUndistMap(0, curr_in[0], curr_in[1]);
        //cv::undistort(*ch,newm, i->cv_cam, i->cv_dist);
        //cv::setNumThreads(0);
          
        for(int c=0;c<curr_in[2];c++)
          remap(cvMat(curr_in.bind(2,c)), cvMat(curr_out.bind(2,c)), *chap, cv::noArray(), cv_interpolation);
        
      }
      else*/
        printf("distortion model not supported\n");
    }
  }
  
  out = curr_out;
  
  if (flags) {
    printf("WARNING: proc_image() did not handle all processing flags!\n ");
  }
}

} //namespace clif
