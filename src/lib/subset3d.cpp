#include "subset3d.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "preproc.hpp"
#include "cam.hpp"

namespace clif {

using namespace std;
using namespace clif;
using namespace cv;

Subset3d::Subset3d(Dataset *set, const cpath & from, const ProcData & proc)
{
  if (!create(set, from, proc))
    abort();
}

bool Subset3d::create(Dataset *set, const cpath & from, const ProcData & proc)
{
  _data = set;
  _proc = proc;
  
  if (from.empty())
    _root = _data->getSubGroup("calibration/extrinsics", from);
  else
    _root = _data->resolve(from);
    
  _store = _data->store(_root/"data");
  
  int skip = 0;
  if (!_store) {
    _root = _data->resolve(from/"extrinsics");
    _store = _data->store(_root/"data");
    
    if (!_store)
      return false;
    
    //FIXME load remaining options!
    Attribute *scale_attr = _data->get(from/"preproc/scale");
    if (scale_attr) {
      float scale;
      scale_attr->get(scale);
      _proc.set_scale(scale);
    }
    Attribute *skip_attr = _data->get(from/"preproc/skip");

    if (skip_attr) {
      skip_attr->get(skip);
      _proc.set_skip(skip);
    }
    Attribute *epi_height_attr = _data->get(from/"preproc/custom_epi_height");
	if (epi_height_attr) {
	  int custom_epi_height;
	  epi_height_attr->get(custom_epi_height);
	  _proc.set_custom_epi_height(custom_epi_height);
	}
  }
  
  _proc.set_flags(_proc.flags() | UNDISTORT);
  _proc.set_store(_store);
  _proc.set_img_count(_store->imgCount());
  
  _data->getEnum((_root/"type"), _type);
  
  //_data->get(root/"world_to_camera", world_to_camera, 6);
    
  if (_type == ExtrType::LINE) {
    double line_step[3];
    
    _data->get(_root/"line_step", line_step, 3);
    
    //TODO for now we only support horizontal lines!
    assert(line_step[0] != 0.0);
    assert(line_step[1] == 0.0);
    assert(line_step[2] == 0.0);

    step_length = line_step[0];
    _proc.set_step_length(step_length);
    //this has to be the original step length that was used for the capture
    //since it is used for the rectification.


    Attribute *focus_point_attr = _data->get(_root/"focus_point");
    if (focus_point_attr) {
    	focus_point_attr->get(focus_point);
    	_proc.set_focus_point(focus_point);
    }

    if (skip)
    	// important for mesh creation
    	step_length *= (skip + 1);
  }
  else if (_type == ExtrType::CIRCLE)
  {
    _data->get(_root/"step_angle", step_length);
  }
  else
  {
    printf("unsupported extrinsics type: %s\n", enum_to_string(_type));
  }
  
  //TODO which extrinsics to select!
  _data->get(_data->getSubGroup("calibration/intrinsics")/"/projection", _f, 2);
  _proc.set_f(_f);
  
  return true;
}


cpath Subset3d::save(Dataset *set, const cpath & to)
{
  cpath to_ = set->resolve(to);
  
  if (to_.empty()) {
    //FIXME check if it exists!
    to_ = "subset/default";
  }
  
  set->addLink(to_/"extrinsics",_root);
  set->setAttribute(to_/"preproc/scale", _proc.scale());
  set->setAttribute(to_/"preproc/skip",  _proc.skip());
  set->setAttribute(to_/"preproc/custom_epi_height",  _proc.custom_epi_height());
  
  return to_;
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


void Subset3d::readEPI(cv::Mat *epi, int line, double disparity, Unit unit)
{
  int w, h, channels, skip;
  double step, depth;
  Idx idx(_store->dims());
  
  Interpolation interp = _proc.interpolation();
  
  if (unit == Unit::PIXELS) {
    step = disparity;
    depth = disparity2depth(disparity);
  }
  else {
    step = depth2disparity(disparity); //f[0]*step_length/disparity*scale;
    depth = disparity;
  }
  
  ProcData proc_curr = _proc;
  proc_curr.set_depth(depth);
  
  bool refocus = true;
  
  //FIXME use info from procdata?
  cpath intrinsics = proc_curr.intrinsics();
  TD_DistType *dtype = dynamic_cast<TD_DistType*>(_data->tree_derive(TD_DistType(intrinsics)));
  if (dtype && dtype->includesRectification())
    refocus = false;
    
  int i_step = step;
  if (abs(i_step - step) < 1.0/512.0)
    interp = Interpolation::NEAREST;
  
  cv::Mat tmp;
  idx[3] = 0;
  //FIXME scale
#pragma omp critical
  _store->readImage(idx, &tmp, proc_curr);
  w = proc_curr.w();
  h = EPIHeight();
  channels = proc_curr.d();

  skip = proc_curr.skip();
  bool debug_skip = false;
  int middle_idx = int(floor(_store->clif::Datastore::imgCount() / 2.0));
  if (debug_skip)
	  std::cout << "epi height:   " << h << std::endl << "middle index: "
		  	<< middle_idx << std::endl;
  int start_idx = 0;
  if (h < _store->clif::Datastore::imgCount()){
	  start_idx = middle_idx - int(floor(h / 2.0)) * (skip + 1);
  }

  if (start_idx < 0) {
	  if (debug_skip)
		std::cout << "WARNING: Skip " << skip
				<< " caused start index to be negative (" << start_idx
				<< "). Setting start index to 0." << std::endl;
	  start_idx = 0;
  }


  int epi_size[3];
  epi_size[2] = w;
  epi_size[1] = h;
  epi_size[0] = channels;
    
  epi->create(3, epi_size, tmp.type());
  
  //TODO fix linear interpolation to reset mat by itself
  if (interp == Interpolation::LINEAR)
    epi->setTo(0);
  
  int cv_t_count = cv::getNumThreads();
  int i_noskip = start_idx; //index in original epi
#pragma omp critical
  if (!cv_t_count)
    cv::setNumThreads(0);
//#pragma omp parallel for schedule(dynamic)
  for(int i=0;i<h;i++) {
    Idx idx_l(_store->dims());
    cv::Mat img;
    idx_l[3] = i_noskip;
    if(debug_skip)
    	std::cout << i_noskip << " ";
#pragma omp critical
    _store->readImage(idx_l, &img, proc_curr);
    
    for(int c=0;c<channels;c++) {
      cv::Mat channel = clifMat_channel(img, c);
      cv::Mat epi_ch = clifMat_channel(*epi, c);

      assert(epi_ch.type() == channel.type());      
      assert(channel.size().width == w);
            
      //FIXME rounding?
      double d = step*(i-h/2);
      
      if (refocus && abs(d) >= w)
        continue;
      
      if (!refocus)
        channel.row(line).copyTo(epi_ch.row(i));
      else {
        if (std::isnan(d) || abs(d) >= w)
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
    i_noskip += skip+1;
  }
  if(debug_skip)
	  std::cout << std::endl;
  
#pragma omp critical
  if (!cv_t_count)
    cv::setNumThreads(cv_t_count);
}


void Subset3d::unshift_epi(cv::Mat *epi, cv::Mat *slice, int line, double disparity, Unit unit)
{
  int w, h, channels;
  double step, depth;
  Idx idx(_store->dims());
  
  Interpolation interp = _proc.interpolation();
  
  if (unit == Unit::PIXELS) {
    step = disparity;
    depth = disparity2depth(disparity);
  }
  else {
    step = depth2disparity(disparity); //f[0]*step_length/disparity*scale;
    depth = disparity;
  }
  
  ProcData proc_curr = _proc;
  proc_curr.set_depth(depth);
  
  bool refocus = true;
  
  //FIXME use path from datastore?
  cpath intrinsics = _data->getSubGroup("calibration/intrinsics");
  TD_DistType *dtype = dynamic_cast<TD_DistType*>(_data->tree_derive(TD_DistType(intrinsics)));
  if (dtype && dtype->includesRectification()) {
    abort();
    //we still need to undo disparity somehow...
    refocus = false;
  }
    
  int i_step = step;
  if (abs(i_step - step) < 1.0/512.0)
    interp = Interpolation::NEAREST;
  
  w = proc_curr.w();
  h = EPIHeight();
  channels = proc_curr.d();
  
  int epi_size[3];
  epi_size[2] = w;
  epi_size[1] = h;
  epi_size[0] = channels;
  
  slice->create(3, epi_size, epi->type());
  slice->setTo(0);
  
  int cv_t_count = cv::getNumThreads();
  
#pragma omp critical
  if (!cv_t_count)
    cv::setNumThreads(0);
//#pragma omp parallel for schedule(dynamic)
  for(int i=0;i<h;i++) {
    for(int c=0;c<channels;c++) {
      cv::Mat slice_ch = clifMat_channel(*slice, c);
      cv::Mat epi_ch = clifMat_channel(*epi, c);
            
      //FIXME rounding?
      double d = -step*(i-h/2);
      
      if (refocus && abs(d) >= w)
        continue;
      
      if (!refocus)
        epi_ch.row(line).copyTo(slice_ch.row(i));
      else {
        if (std::isnan(d) || abs(d) >= w)
          continue;
        
        switch (interp) {
          case Interpolation::LINEAR :
            callByBaseType<warp_1d_linear_dispatcher>(CvDepth2BaseType(epi_ch.depth()), &epi_ch, &slice_ch, line, i, d);
            break;
          case Interpolation::NEAREST :
          default :
            callByBaseType<warp_1d_nearest_dispatcher>(CvDepth2BaseType(epi_ch.depth()),&epi_ch, &slice_ch, i, i, d);
        }
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
  return _proc.h();
}

int Subset3d::EPIDepth()
{
  return _proc.d();
}

int Subset3d::EPIWidth()
{
  //FIXME depends on rotation!
  return _proc.w();
}

int Subset3d::EPIHeight()
{
  //FIXME use extrinsics group size! (for cross type...)  
	int height = 0;
	int max_height = int(ceil(_store->clif::Datastore::imgCount() / (_proc.skip() + 1.0)));
	if (_proc.custom_epi_height())
		height = std::min(_proc.custom_epi_height(), max_height);
	else
		height = max_height;
	return height;
}

cpath Subset3d::extrinsics_group()
{
  return _root;
}

}
