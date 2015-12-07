#include "preproc.hpp"

#include "mat.hpp"
#include "datastore.hpp"

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
    printf("curr in data: %p = %p\n", curr_in.data(), curr_out.data());
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
  
void proc_image(Datastore *store, Mat &in, Mat &out, int flags, double min, double max, int scaledown)
{
  int flags_remain;
  Mat curr_in;
  Mat curr_out = in;
  
  //FIXME hdr may need 4!
  assert(in.size() == 3);
  
  if (flags & Improc::DEMOSAIC) {
    printf("FIXME handle demosaicing!\n");
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
    
    cvMat(curr_in).convertTo(cvMat(curr_out), CV_8U, BaseType_max<uint8_t>()/BaseType_max(curr_in.type()));
  }
  
  if (flags & Improc::UNDISTORT) {
    printf("FIXME handle undistortion!\n");
  }
  
  out = curr_out;
  
  if (flags) {
    printf("WARNING: proc_image() did not handle all processing flags!\n ");
  }
}

} //namespace clif
