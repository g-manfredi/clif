#ifndef _CLIF_PREPROC_H
#define _CLIF_PREPROC_H

#include "mat.hpp"

#include <limits>
#include <functional>

#include <opencv2/core/core.hpp>

namespace clif {
  
class Datastore;

enum Improc {
  DEMOSAIC = 1, 
  CVT_8U = 2,
  CVT_GRAY = 8,
  EXP_STACK = 16,
  UNDISTORT = 32,
  HQ = 64,
  NO_DISK_CACHE = 128,
  NO_MEM_CACHE = 256,
  FORCE_REUSE = 512,
  MAX = 1024,
  _SCALE = 2048
};

#define ACC_D(T,V,D) private: T _##V = D; public: const T & V() const { return _##V; } void set_ ## V(T val) { _##V = val; }
#define ACC_D_P(T,V,D) private: T _##V = D; public: T V() const { return _##V; } void set_ ## V(T val) { _##V = val; }
#define ACC(T,V) private: T _##V; public: const T & V() const { return _##V; } void set_ ## V(T val) { _##V = val; }

class Datastore;

class ProcData
{
public:
  ProcData() {};
  ProcData(Datastore *store,
           int flags,
           double min = std::numeric_limits<double>::quiet_NaN(),
           double max = std::numeric_limits<double>::quiet_NaN(),
           double depth = std::numeric_limits<double>::quiet_NaN(),
           Interpolation interp = Interpolation::LINEAR,
           float scale = 1.0,
           int every = 1
          );
  ACC_D(int, every, 1)
  ACC_D(double, min, std::numeric_limits<double>::quiet_NaN())
  ACC_D(double, max, std::numeric_limits<double>::quiet_NaN())
  ACC_D(double, depth, std::numeric_limits<double>::quiet_NaN())
  ACC_D(float, scale, 1.0)
  ACC_D(Interpolation, interpolation, Interpolation::LINEAR)
  ACC(cpath, intrinsics)
  
public:
  Datastore* store() const;
  void set_store(Datastore *store);
  int flags() const;
  void set_flags(int flags);
  
  int w() const;
  int h() const;
  int d() const;
  
  BaseType type();
  
  inline int scale(int val) const
  {
    return cvRound(val*_scale);
  }
  
private:
  Datastore *_store = NULL;
  int _w = 0;
  int _h = 0;
  int _d = 0;
  int _flags = 0;
};

#undef ACC

//if input != output then max defaults to input max type
//flt/dbl output is scaled then to 0..1
void proc_image(Datastore *store, Mat &in, Mat &out, int flags, const Idx & pos = Idx(), double min = std::numeric_limits<double>::quiet_NaN(), double max = std::numeric_limits<double>::quiet_NaN(), int scaledown = 0, double depth = std::numeric_limits<double>::quiet_NaN());

void proc_image(Mat &in, Mat &out, const ProcData & proc, const Idx & pos = Idx());

} //namespace clif

#endif //_CLIF_PREPROC_H