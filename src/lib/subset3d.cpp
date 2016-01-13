#include "subset3d.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "preproc.hpp"

namespace clif {

using namespace std;
using namespace clif;
using namespace cv;

Subset3d::Subset3d(Dataset *data, cpath extr_group)
{
  create(data, extr_group);
}

void Subset3d::create(Dataset *data, cpath extr_group)
{
  _data = data;
  
  cpath root = _data->getSubGroup("calibration/extrinsics", extr_group);
  _store = _data->getStore(root/"data");
  
  _data->getEnum((root/"type"), _type);
  
  //_data->get(root/"world_to_camera", world_to_camera, 6);
    
  if (_type == ExtrType::LINE) {
    double line_step[3];
    
    _data->get(root/"/line_step", line_step, 3);
    
    //TODO for now we only support horizontal lines!
    assert(line_step[0] != 0.0);
    assert(line_step[1] == 0.0);
    assert(line_step[2] == 0.0);

    step_length = line_step[0];
  }
  else if (_type == ExtrType::CIRCLE)
  {
    _data->get(root/"step_angle", step_length);
  }
  else
  {
    printf("unsupported extrinsics type: %s\n", enum_to_string(_type));
  }
  
  //TODO which extrinsics to select!
  _data->get(_data->getSubGroup("calibration/intrinsics")/"/projection", f, 2);
}

ExtrType Subset3d::type()
{
  return _type;
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
  
  int start = 0;
  int stop = out->size().width;
  
  if (off_i > 0)
    start = off_i;
  if (stop - off_i + 1 > out->size().width)
    stop = out->size().width + off_i - 1;
  
  for(int i=start;i<stop;i++)
    out_ptr[i] = (f1*(int)in_ptr[i-off_i] + f2*(int)in_ptr[i-off_i+1])/1024;
  for(int i=stop;i<out->size().width;i++)
    out_ptr[i] = 0;
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
  
  int start = 0;
  int stop = out->size().width;
  
  if (offset > 0)
    start = offset;
  if (stop - offset > out->size().width)
    stop = out->size().width + offset;
  
  int size = stop - start;
  
  if (start)
    memset(out_ptr, 0, start*sizeof(T));
  memcpy(out_ptr+start, in_ptr+start-offset, size*sizeof(T));
  if (stop < out->size().width)
    memset(out_ptr+stop, 0, (out->size().width-stop)*sizeof(T));
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
  double step, depth;
  Idx idx(_store->dims());
  
  if (unit == Unit::PIXELS) {
    step = disparity;
    depth = disparity2depth(disparity);
  }
  else {
    step = depth2disparity(disparity, scale); //f[0]*step_length/disparity*scale;
    depth = disparity;
  }
    
  int i_step = step;
  if (abs(i_step - step) < 1.0/512.0)
    interp = Interpolation::NEAREST;
  
  cv::Mat tmp;
  idx[3] = 0;
  //FIXME scale
  _store->readImage(idx, &tmp, flags | Improc::UNDISTORT, depth);
  w = tmp.size[2];
  h = _store->clif::Datastore::imgCount();
  
  int epi_size[3];
  epi_size[2] = w;
  epi_size[1] = h;
  epi_size[0] = clifMat_channels(tmp);
  
  epi->create(3, epi_size, tmp.type());
  
  //TODO fix linear interpolation to reset mat by itself
  //if (interp == Interpolation::LINEAR)
    epi->setTo(0);
  
  int cv_t_count = cv::getNumThreads();
  
#pragma omp critical
  if (!cv_t_count)
    cv::setNumThreads(0);
//#pragma omp parallel for schedule(dynamic)
  for(int i=0;i<h;i++) {
    Idx idx_l(_store->dims());
    cv::Mat img;
    idx_l[3] = i;
    _store->readImage(idx_l, &img, flags | UNDISTORT, depth);
    
#pragma omp critical 
    if (i == 0)
    {
    cv::Mat col;
    clifMat2cv(&img, &col);
    imwrite("debug.tif", col);
    }
    
    for(int c=0;c</*_store->imgChannels()*/3;c++) {
      cv::Mat channel = clifMat_channel(img, c);
      cv::Mat epi_ch = clifMat_channel(*epi, c);

      assert(epi_ch.type() == channel.type());      
      assert(channel.size().width == w);
            
      //FIXME rounding?
      double d = step*(i-h/2);
      
      //if (abs(d) >= w)
        //continue;
      
      channel.row(line).copyTo(epi_ch.row(i));
      
      /*switch (interp) {
        case Interpolation::LINEAR :
          callByBaseType<warp_1d_linear_dispatcher>(CvDepth2BaseType(epi_ch.depth()), &channel, &epi_ch, line, i, d);
          break;
        case Interpolation::NEAREST :
        default :
          callByBaseType<warp_1d_nearest_dispatcher>(CvDepth2BaseType(epi_ch.depth()),&channel, &epi_ch, line, i, d);
      }*/
    }
  }
  
  
    cv::Mat col;
    clifMat2cv(epi, &col);
    imwrite("epi.tif", col);
  
#pragma omp critical
  if (!cv_t_count)
    cv::setNumThreads(cv_t_count);
}

int Subset3d::EPICount()
{
  //FIXME use extrinsics group size! (for cross type...)  
  //FIXME depends on rotation!
  return _store->extent()[1];
}

int Subset3d::EPIWidth()
{
  //FIXME depends on rotation!
  return _store->extent()[0];
}

int Subset3d::EPIHeight()
{
  //FIXME use extrinsics group size! (for cross type...)  
  return _store->clif::Datastore::imgCount();
}

}
