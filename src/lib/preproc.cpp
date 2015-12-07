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
/*
#define CHECK_FLAG_PREPARE(FLAG) if (flags & FLAG) { \
    curr_in = curr_out; \
    if (!(flags ^ FLAG)) \
      //write to final output \
      curr_out = out; \
    else \
      //use new matrix \
      curr_out.release(); \
    } \
    if (flags & FLAG) \*/

  
void proc_image(Datastore *store, Mat &in, Mat &out, int flags, int scaledown, double min, double max)
{
  int flags_remain;
  Mat curr_in;
  Mat curr_out = in;
  
  assert(in.size() == 3);
  
  /*CHECK_FLAG_PREPARE(CVT_GRAY) {
    cv::Mat accumulate;
    
    for(int c=0;c<in[2];c++) {
      cv::Mat ch = cvMat(curr_in.bind(2, c));
    }
  }*/
}

} //namespace clif
