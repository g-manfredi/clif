#include "subset3d.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

namespace clif {

using namespace std;
using namespace clif;
using namespace cv;

void Subset3d::create(Dataset *data, std::string extr_group)
{
  _data = data;
  
  cpath root = _data->subGroupPath("calibration/extrinsics", extr_group);
  
  ExtrType type;
  
  _data->getEnum((root/"type"), type);
  
  assert(type == ExtrType::LINE);
  
  double line_step[3];
  
  _data->get(root/"/line_step", line_step, 3);
  
  //TODO for now we only support horizontal lines!
  assert(line_step[0] != 0.0);
  assert(line_step[1] == 0.0);
  assert(line_step[2] == 0.0);

  step_length = line_step[0];
  
  //TODO which intrinsic to select!
  _data->get(_data->subGroupPath("calibration/intrinsics/")/"/projection", f, 2);
}


Subset3d::Subset3d(Dataset *data, std::string extr_group)
{  
  create(data, extr_group);
}

Subset3d::Subset3d(clif::Dataset *data, const int idx)
{
  std::vector<std::string> subs;
  
  data->listSubGroups("calibration/extrinsics", subs);
  
  assert((int)subs.size() >= idx);
  
  create(data, subs[idx]);
}


typedef Vec<ushort, 3> Vec3us;

template<typename V> void warp_1d_linear_rgb(cv::Mat in, cv::Mat out, double offset)
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
  
  for(int i=clamp<int>(off_i, 1, out.size().width-1);i<clamp<int>(out.size().width+off_i, 1, out.size().width);i++) {
    out_ptr[i] = (f1*(Vec3i)in_ptr[i-off_i] + f2*(Vec3i)in_ptr[i-off_i+1])/1024;
  }
  /*for(int i=0;i<out.size().width;i++) {
    out_ptr[i] = in_ptr[i];
  }*/
}

//FIXME fixed point only!?
/*template<typename T> void warp_1d_linear(Mat *in, Mat *out, int line_in, int line_out, double offset)
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
  
  for(int i=clamp<int>(off_i, 1, out->size().width-1);i<clamp<int>(out->size().width+off_i, 1, out->size().width);i++)
    out_ptr[i] = (f1*(int)in_ptr[i-off_i] + f2*(int)in_ptr[i-off_i+1])/1024;
}*/

//FIXME fixed point only!?
template<typename T> void warp_1d_linear(cv::Mat *in, cv::Mat *out, int line_in, int line_out, double offset)
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
  
  for(int i=clamp<int>(off_i, 1, out->size().width-1);i<clamp<int>(out->size().width+off_i, 1, out->size().width);i++)
    out_ptr[i] = (f1*(int)in_ptr[i-off_i] + f2*(int)in_ptr[i-off_i+1])/1024;
}

template<typename T> class warp_1d_linear_dispatcher {
public:
  void operator()(cv::Mat *in, cv::Mat *out, int line_in, int line_out, double offset)
  {
    warp_1d_linear<T>(in, out, line_in, line_out, offset);
  }
};
template<typename T> class warp_1d_linear_dispatcher<std::vector<T>> {
public:
  void operator()(cv::Mat *in, cv::Mat *out, int line_in, int line_out, double offset)
  {
    abort();
  }
};
template<> class warp_1d_linear_dispatcher<cv::Point2f> {
public:
  void operator()(cv::Mat *in, cv::Mat *out, int line_in, int line_out, double offset)
  {
    abort();
  }
};

template<typename T> void warp_1d_nearest(cv::Mat *in, cv::Mat *out, int line_in, int line_out, int offset)
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
  void operator()(cv::Mat *in, cv::Mat *out, int line_in, int line_out, int offset)
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


void Subset3d::readEPI(cv::Mat *epi, int line, double disparity, Unit unit, int flags, Interpolation interp, float scale)
{
  int w, h;
  double step;
  std::vector<int> idx(_data->dims(), 0);
  
  if (unit == Unit::PIXELS)
    step = disparity;
  else
    step = depth2disparity(disparity, scale); //f[0]*step_length/disparity*scale;
  
  int i_step = step;
  if (abs(i_step - step) < 1.0/512.0)
    interp = Interpolation::NEAREST;
  
  cv::Mat tmp;
  idx[3] = 0;
  //FIXME scale
  _data->readImage(idx, &tmp, flags | UNDISTORT);
  w = tmp.size[2];
  h = _data->clif::Datastore::imgCount();
  
  int epi_size[3];
  epi_size[2] = w;
  epi_size[1] = h;
  epi_size[0] = clifMat_channels(tmp);
  
  epi->create(3, epi_size, tmp.type());
  
  //TODO fix linear interpolation to reset mat by itself
  if (interp == Interpolation::LINEAR)
    epi->setTo(0);
  
  int cv_t_count = cv::getNumThreads();
  
#pragma omp critical
  if (!cv_t_count)
    cv::setNumThreads(0);
#pragma omp parallel for schedule(dynamic)
  for(int i=0;i<h;i++) {
    std::vector<int> idx_l(_data->dims(), 0);
    cv::Mat img;
    idx_l[3] = i;
    _data->readImage(idx_l, &img, flags | UNDISTORT);
    
    for(int c=0;c<_data->imgChannels();c++) {    
      cv::Mat channel = clifMat_channel(img, c);
      cv::Mat epi_ch = clifMat_channel(*epi, c);

      assert(epi_ch.type() == channel.type());      
      assert(channel.size().width == w);
            
      //FIXME rounding?
      double d = step*(i-h/2);
      
      if (abs(d) >= w)
        continue;
      
      switch (interp) {
        case Interpolation::LINEAR :
          callByBaseType<warp_1d_linear_dispatcher>(CvDepth2BaseType(epi_ch.depth()), &channel, &epi_ch, line, i, d);
          break;
        case Interpolation::NEAREST :
        default :
          callByBaseType<warp_1d_nearest_dispatcher>(CvDepth2BaseType(epi_ch.depth()),&channel, &epi_ch, line, i, d);
      }
    }
  }
#pragma omp critical
  if (!cv_t_count)
    cv::setNumThreads(cv_t_count);
}

int Subset3d::EPICount()
{
  //FIXME use extrinsics group size! (for cross type...)  
  //FIXME depends on rotation!
  return _data->extent()[1];
}

int Subset3d::EPIWidth()
{
  //FIXME depends on rotation!
  return _data->extent()[0];
}

int Subset3d::EPIHeight()
{
  //FIXME use extrinsics group size! (for cross type...)  
  return _data->clif::Datastore::imgCount();
}

}
