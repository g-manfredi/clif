#include "subset3d.hpp"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

namespace clif {

using namespace std;
using namespace clif;
using namespace cv;

Subset3d::Subset3d(Dataset *data, std::string extr_group)
: _data(data)
{  
  path root = data->subGroupPath("calibration/extrinsics", extr_group);
  
  ExtrType type;
  
  data->getEnum((root/"type"), type);
  
  assert(type == ExtrType::LINE);
  
  double line_step[3];
  
  data->getAttribute(root/"/line_step", line_step, 3);
  
  //TODO for now we only support horizontal lines!
  assert(line_step[0] != 0.0);
  assert(line_step[1] == 0.0);
  assert(line_step[2] == 0.0);

  step_length = line_step[0];
  
  //TODO which intrinsic to select!
  data->getAttribute(data->subGroupPath("calibration/intrinsics/")/"/projection", f, 2);
}


typedef Vec<ushort, 3> Vec3us;

template<typename V> void warp_1d_linear_rgb(Mat in, Mat out, double offset)
{
  
  if (abs(offset) >= in.size().width) {
    return;
  }
  
  V *in_ptr = in.ptr<V>(0);
  V *out_ptr = out.ptr<V>(0);
  
  int off_i;
  int f1, f2;
  
  if (offset >= 0)
    off_i = offset;
  else
    off_i = offset-1;
  
  f1 = (offset - (double)off_i)*1024;
  f2 = 1024 - f1;
  
  for(int i=clamp<int>(off_i, 1, out.size().width-1);i<clamp<int>(out.size().width+off_i, 1, out.size().width-1);i++) {
    out_ptr[i] = (f1*(Vec3i)in_ptr[i-off_i] + f2*(Vec3i)in_ptr[i-off_i+1])/1024;
  }
  /*for(int i=0;i<out.size().width;i++) {
    out_ptr[i] = in_ptr[i];
  }*/
}

template<typename T> void warp_1d_linear(Mat *in, Mat *out, int line_in, int line_out, int offset)
{
  
  if (abs(offset) >= in->size().width) {
    return;
  }
  
  T *in_ptr = (T*)in->ptr<T>(line_in);
  T *out_ptr = (T*)out->ptr<T>(line_out);
  
  int off_i;
  int f1, f2;
  
  if (offset >= 0)
    off_i = offset;
  else
    off_i = offset-1;
  
  f1 = (offset - (double)off_i)*1024;
  f2 = 1024 - f1;
  
  for(int i=clamp<int>(off_i, 1, out->size().width-1);i<clamp<int>(out->size().width+off_i, 1, out->size().width-1);i++) {
    out_ptr[i] = (f1*(int)in_ptr[i-off_i] + f2*(int)in_ptr[i-off_i+1])/1024;
  }
}

template<typename T> class warp_1d_linear_dispatcher {
public:
  void operator()(Mat *in, Mat *out, int line_in, int line_out, int offset)
  {
    warp_1d_linear<T>(in, out, line_in, line_out, offset);
  }
};

template<typename T> void warp_1d_nearest(Mat *in, Mat *out, int line_in, int line_out, int offset)
{
  
  if (abs(offset) >= in->size().width) {
    return;
  }
  
  T *in_ptr = (T*)in->ptr<T>(line_in);
  T *out_ptr = (T*)out->ptr<T>(line_out);
  
  int start = clamp<int>(offset, 0, out->size().width);
  int size = clamp<int>(out->size().width+offset, 0, out->size().width) - start;
  
  if (start)
    memset(out_ptr, 0, start*sizeof(T));
  memcpy(out_ptr+start, in_ptr+start-offset, size*sizeof(T));
  if (out->size().width-size-start)
    memset(out_ptr+(start+size)*sizeof(T), 0, (out->size().width-size-start)*sizeof(T));
}

template<typename T> class warp_1d_nearest_dispatcher {
public:
  void operator()(Mat *in, Mat *out, int line_in, int line_out, int offset)
  {
    warp_1d_nearest<T>(in, out, line_in, line_out, offset);
  }
};
/*
template<typename T> void warp_1d_nearest(Mat in, Mat out, int offset)
{
  
  if (abs(offset) >= in.size().width) {
    return;
  }
  
  T *in_ptr = in.ptr<T>(0);
  T *out_ptr = out.ptr<T>(0);
  
  int start = clamp<int>(offset, 1, out.size().width-1) * sizeof(T);
  int size = clamp<int>(out.size().width+offset, 0, out.size().width) * sizeof(T) - start;
  
  memcpy(out_ptr+start, in_ptr+start-offset, size);
}*/

void Subset3d::readEPI(cv::Mat &m, int line, double disparity, ClifUnit unit, int flags, Interpolation interp, float scale)
{
  double step;
  
  if (unit == ClifUnit::PIXELS)
    step = disparity;
  else
    step = depth2disparity(disparity, scale); //f[0]*step_length/disparity*scale;
  
  cv::Mat tmp;
  readCvMat(_data, 0, tmp, flags | UNDISTORT, scale);

  m = cv::Mat::zeros(cv::Size(tmp.size().width, _data->clif::Datastore::count()), tmp.type());
  
  for(int i=0;i<_data->clif::Datastore::count();i++)
  {      
    //FIXME rounding?
    double d = step*(i-_data->clif::Datastore::count()/2);
    
    if (abs(d) >= tmp.size().width)
      continue;
    
    readCvMat(_data, i, tmp, flags | UNDISTORT, scale);
    
    if (tmp.type() == CV_16UC3)
      warp_1d_linear_rgb<Vec3us>(tmp.row(line), m.row(i), d);
    else if (tmp.type() == CV_8UC3)
      warp_1d_linear_rgb<Vec3b>(tmp.row(line), m.row(i), d);
    //Matx23d warp(1,0,d, 0,1,0);
    //warpAffine(tmp.row(line), m.row(i), warp, m.row(i).size(), CV_INTER_LANCZOS4);
  }
}


void Subset3d::readEPI(std::vector<cv::Mat> &channels, int line, double disparity, ClifUnit unit, int flags, Interpolation interp, float scale)
{
  int w, h;
  double step;
  
  if (unit == ClifUnit::PIXELS)
    step = disparity;
  else
    step = depth2disparity(disparity, scale); //f[0]*step_length/disparity*scale;
  
  int i_step = step;
  if (abs(i_step - step) < 1.0/512.0)
    interp = Interpolation::NEAREST;
  
  std::vector<cv::Mat> tmp;
  readCvMat(_data, 0, tmp, flags | UNDISTORT, scale);
  w = tmp[0].size().width;
  h = _data->clif::Datastore::count();

  channels.resize(tmp.size());
  
  for(int c=0;c<channels.size();c++) {
    channels[c] = cv::Mat(cv::Size(w, h), tmp[0].type());
    Mat *ch = &channels[c];
    
    //TODO fix linear interpolation to reset mat by itself
    if (interp == Interpolation::LINEAR)
      channels[c] = cv::Mat::zeros(cv::Size(w, h), tmp[0].type());
    
    for(int i=0;i<h;i++)
    {      
      //FIXME rounding?
      double d = step*(i-h/2);
      
      if (abs(d) >= w)
        continue;
      
      //Mat line_in = tmp[c].row(line);
      //Mat line_out = ch->row(i);
      
      switch (interp) {
        case Interpolation::LINEAR :
          _data->call<warp_1d_linear_dispatcher>(&tmp[c], ch, line, i, d);
          break;
        case Interpolation::NEAREST :
        default :
          _data->call<warp_1d_nearest_dispatcher>(&tmp[c], ch, line, i, d);
      }
      
      /*if (tmp[c].type() == CV_16UC1)
        //warp_1d_nearest<uint16_t>(tmp[c].row(line), ch->row(i), d);
        warp_1d_linear<uint16_t>(tmp[c].row(line), ch->row(i), d);
      else if (tmp[c].type() == CV_8UC1)
        warp_1d_linear<uint8_t>(tmp[c].row(line), ch->row(i), d);
      else
        abort();*/
      //Matx23d warp(1,0,d, 0,1,0);
      //warpAffine(tmp.row(line), m.row(i), warp, m.row(i).size(), CV_INTER_LANCZOS4);
    }
  }
}

}