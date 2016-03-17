
#include "dataset.hpp"
#include "calib.hpp"
#include "clif_cv.hpp"
#include "enumtypes.hpp"
#include "preproc.hpp"

#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef CLIF_WITH_HDMARKER
  #include <hdmarker/hdmarker.hpp>
  #include <hdmarker/subpattern.hpp>
  
  using namespace hdmarker;
#endif
  
#ifdef CLIF_WITH_UCALIB
  #include <ucalib/corr_lines.hpp>
  #include <ucalib/gencam.hpp>
  
  #include "ceres/ceres.h"
  #include "ceres/rotation.h"
  
  const double proj_center_size = 0.1;
  const double extr_center_size = 0.2;
  const double non_center_rest_weigth = 1e-4;
  const double strong_proj_constr_weight = 10.0;
  const double proj_constr_weight = 1e-4;
  const double center_constr_weight = 0.1;
  const double enforce_cams_line = 10.0;
  
  int _calib_cams_limit = 1000;
  int _calib_views_limit = 1000;
#endif
  
#ifdef CLIF_WITH_LIBIGL_VIEWER
  #include "mesh.hpp"
  #include <igl/viewer/Viewer.h>
  #include <thread>
#endif
  
#include "mat.hpp"
  
using namespace std;
using namespace cv;

namespace clif {
  
typedef unsigned int uint;

bool pattern_detect(Dataset *s, cpath imgset, cpath calibset, bool write_debug_imgs)
{
  cpath img_root, map_root;
  bool wtf;
  wtf = s->deriveGroup("calibration/imgs", imgset, "calibration/mapping", calibset, img_root, map_root);
  std::cout << wtf << "\n";
  if (!wtf)
    //abort();
    printf("should actually abort!\n");
  else
    printf("all good\n");
  
  Datastore *debug_store = NULL;

  CalibPattern pattern;
  s->getEnum(img_root/"type", pattern);
  
  if (write_debug_imgs && pattern == CalibPattern::HDMARKER)
    debug_store = s->getStore(map_root / "data");
  else
    write_debug_imgs = false;
    
  //FIXME implement generic "format" class?
  Datastore *imgs = s->getStore(img_root/"data");
  assert(imgs);
    
  vector<vector<Point2f>> ipoints;
  vector<vector<Point2f>> wpoints;
  
  //FIXME grayscale for opencv calib!
  int readflags = CVT_8U | DEMOSAIC;
  
  if (pattern == CalibPattern::CHECKERBOARD)
    readflags |= CVT_GRAY;
  
  //FIXME integrate ProcData!
  ProcData proc(readflags, imgs);
  
  int channels = proc.d();
  
  Idx map_size(imgs->dims() - 2);
  map_size[0] = channels;
  for(int i=1;i<map_size.size();i++)
    map_size[i] = imgs->extent()[i+2];
  
  Idx pos(map_size.size());

  
  Mat_<std::vector<Point2f>> wpoints_m(map_size);
  Mat_<std::vector<Point2f>> ipoints_m(map_size);
  
  if (pattern == CalibPattern::CHECKERBOARD) {
    cv::Mat img;
    int size[2];
    
    s->get(img_root / "size", size, 2);
  
    for(;pos<map_size;pos.step(1, map_size)) {
      vector<Point2f> corners;
      std::vector<int> idx(imgs->dims());
      for(int i=3;i<idx.size();i++)
        idx[i] = pos[i-2];
      imgs->readImage(idx, &img, readflags);
      
      cv::Mat ch = clifMat_channel(img, 0);
      
      int succ = findChessboardCorners(ch, Size(size[0],size[1]), corners, CV_CALIB_CB_ADAPTIVE_THRESH+CV_CALIB_CB_NORMALIZE_IMAGE+CALIB_CB_FAST_CHECK+CV_CALIB_CB_FILTER_QUADS);
      
      if (succ) {
        printf("found %6lu corners\n", corners.size());
        cornerSubPix(ch, corners, Size(8,8), Size(-1,-1), TermCriteria(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS,100,0.0001));
      }
      else
        printf("found      0 corners\n");
      
      ipoints.push_back(std::vector<Point2f>());
      wpoints.push_back(std::vector<Point2f>());
      
      if (succ) {
        for(int y=0;y<size[1];y++)
          for(int x=0;x<size[0];x++) {
            ipoints.back().push_back(corners[y*size[0]+x]);
            wpoints.back().push_back(Point2f(x,y));
            //pointcount++;
          }
          
        //at the moment single color only!
        pos[0] = 0;
          
        wpoints_m(pos) = wpoints.back();
        ipoints_m(pos) = ipoints.back();
      }
    }
  }
#ifdef CLIF_WITH_HDMARKER
  else if (pattern == CalibPattern::HDMARKER) {
    
    double unit_size; //marker size in mm
    double unit_size_res;
    int recursion_depth;
    cv::Rect limit;
    
    s->get(img_root / "marker_size", unit_size);
    s->get(img_root / "hdmarker_recursion", recursion_depth);
    
    Attribute *bbox_a = s->get(img_root/"bbox");
    if (bbox_a) {
      int bbox_vals[4];
      bbox_a->get(bbox_vals, 4);
      limit = cv::Rect(bbox_vals[0],bbox_vals[1],bbox_vals[2],bbox_vals[3]);
    }
  
    
    //FIXME remove this!
    Marker::init();

    for(;pos<map_size;pos.step(1, map_size)) {
      cout << "processing: " << pos << " of " << map_size << "\n";
      std::vector<int> idx(imgs->dims());
      for(int i=3;i<idx.size();i++)
        idx[i] = pos[i-2];
    
      std::vector<Corner> corners_rough;
      std::vector<Corner> corners;

      bool *mask_ptr = NULL;
      bool masks[3][4];
      
      cv::Mat *debug_img_ptr = NULL;
      cv::Mat debug_img;          
      
      if (imgs->org() == DataOrg::BAYER_2x2) {        
        for(int mc=0;mc<3;mc++)
          for(int m=0;m<4;m++)
            masks[mc][m] = false;
          
          switch (imgs->order()) {
            case DataOrder::RGGB : 
              masks[0][0] = true;
              masks[1][1] = true;
              masks[1][2] = true;
              masks[2][3] = true;
              break;
            case DataOrder::GBRG : 
              masks[0][2] = true; 
              masks[1][0] = true;
              masks[1][3] = true;
              masks[2][1] = true;
              break;
            default :
              abort();
          }
          
          printf("process %d x %d\n", pos[1], pos[2]);
          
          cv::Mat debug_imgs[3];
          
          //grayscale rough detection
          //FIXME move this up - mmapped reallocation not possible...
          cv::Mat img;
          imgs->readImage(idx, &img, readflags);
          cv::Mat gray = clifMat_channel(img, 0);
          Marker::detect(gray, corners_rough);
          
          cv::Mat bayer_img;
          imgs->readImage(idx, &bayer_img, CVT_8U);
          cv::Mat bayer = clifMat_channel(bayer_img, 0);
          
          char buf[128];

          for(int c=0;c<channels;c++) {
            if (debug_store)
              debug_img_ptr = &debug_imgs[c];
            
            pos[0] = c;
            
            unit_size_res = unit_size;
            mask_ptr = &masks[c][0];
            hdmarker_detect_subpattern(bayer, corners_rough, corners, recursion_depth, &unit_size_res, debug_img_ptr, mask_ptr, 0, limit);
            
            printf("found %6lu corners for channel %d\n", corners.size(), c);
            
            //sprintf(buf, "debug_img%03d_ch%d.tif", j, c);
            //imwrite(buf, *debug_img_ptr);
            
            std::vector<Point2f> ipoints_v(corners.size());
            std::vector<Point2f> wpoints_v(corners.size());
            
            for(int ci=0;ci<corners.size();ci++) {
              //FIXME multi-channel calib!
              ipoints_v[ci] = corners[ci].p;
              Point2f w_2d = unit_size_res*Point2f(corners[ci].id.x, corners[ci].id.y);
              wpoints_v[ci] = Point2f(w_2d.x, w_2d.y);
            }
            
            wpoints_m(pos) = wpoints_v;
            ipoints_m(pos) = ipoints_v;
            s->flush();
          }
          
          if (debug_store)
            cv::merge(debug_imgs, 3, debug_img);
      }
      else {
        cv::Mat debug_imgs[proc.d()];
        
        //grayscale rough detection
        //FIXME move this up - mmapped reallocation not possible...
        cv::Mat img;
        imgs->readImage(idx, &img, readflags);
        cv::Mat gray = clifMat_channel(img, 0);
        Marker::detect(gray, corners_rough);
        
        cv::Mat img_color;
        imgs->readImage(idx, &img_color, CVT_8U);
        
        
        for(int c=0;c<channels;c++) {
          if (debug_store)
            debug_img_ptr = &debug_imgs[c];
          
          pos[0] = c;
          
          cv::Mat ch = clifMat_channel(img_color, 0);
          
          unit_size_res = unit_size;
          hdmarker_detect_subpattern(ch, corners_rough, corners, recursion_depth, &unit_size_res, debug_img_ptr, NULL, 0, limit);
          
          printf("found %6lu corners for channel %d\n", corners.size(), c);
          
          //char buf[128];
          //sprintf(buf, "debug_img%03d_ch%d.tif", j, c);
          //imwrite(buf, *debug_img_ptr);
          
          std::vector<Point2f> ipoints_v(corners.size());
          std::vector<Point2f> wpoints_v(corners.size());
          
          for(int ci=0;ci<corners.size();ci++) {
            //FIXME multi-channel calib!
            ipoints_v[ci] = corners[ci].p;
            Point2f w_2d = unit_size_res*Point2f(corners[ci].id.x, corners[ci].id.y);
            wpoints_v[ci] = Point2f(w_2d.x, w_2d.y);
          }
          
          wpoints_m(pos) = wpoints_v;
          ipoints_m(pos) = ipoints_v;
          s->flush();
        }
        
        if (debug_store) {
          if (proc.d() == 1)
            debug_img = debug_imgs[0];
          else
            cv::merge(debug_imgs, proc.d(), debug_img);
        }
      }
      
      if (debug_store) {
        debug_store->appendImage(&debug_img);
        s->flush();
        
        //char buf[128];
        //sprintf(buf, "col_fit_img%03d.tif", j);
        //imwrite(buf, debug_img);
      }
    }
  }
  #endif
  else
    abort();
  
  s->setAttribute(map_root / "img_points", ipoints_m);
  s->setAttribute(map_root / "world_points", wpoints_m);
  
  std::vector<int> imgsize = { imgSize(imgs).width, imgSize(imgs).height };
  
  s->setAttribute(map_root / "img_size", imgsize);
  
  return false;
}

bool opencv_calibrate(Dataset *set, int flags, cpath map, cpath calib)
{
  cpath map_root, calib_root;
  if (!set->deriveGroup("calibration/mapping", map, "calibration/intrinsics", calib, map_root, calib_root))
    abort();
  
  cv::Mat cam;
  vector<double> dist;
  vector<cv::Mat> rvecs;
  vector<cv::Mat> tvecs;
  int im_size[2];
  
  Mat_<std::vector<Point2f>> wpoints_m;
  Mat_<std::vector<Point2f>> ipoints_m;
    
  vector<vector<Point2f>> ipoints;
  vector<vector<Point3f>> wpoints;
  
  Attribute *w_a = set->get(map_root/"world_points");
  Attribute *i_a = set->get(map_root/"img_points");
  set->get(map_root/"img_size", im_size, 2);
  
  //FIXME error handling
  if (!w_a || !i_a)
    abort();
  
  w_a->get(wpoints_m);
  i_a->get(ipoints_m);
  
  for(int i=0;i<wpoints_m[1];i++) {
    if (!wpoints_m(0, i).size())
      continue;
    
    ipoints.push_back(ipoints_m(0, i));
    wpoints.push_back(std::vector<Point3f>(wpoints_m(0, i).size()));
    for(int j=0;j<wpoints_m(0, i).size();j++)
      wpoints.back()[j] = Point3f(wpoints_m(0, i)[j].x,wpoints_m(0, i)[j].y,0);
  }
  
  double rms = calibrateCamera(wpoints, ipoints, cv::Size(im_size[0],im_size[1]), cam, dist, rvecs, tvecs, flags);
  
  
  printf("opencv calibration rms %f\n", rms);
  
  std::cout << cam << std::endl;
  
  double f[2] = { cam.at<double>(0,0), cam.at<double>(1,1) };
  double c[2] = { cam.at<double>(0,2), cam.at<double>(1,2) };
  
  //FIXME todo delete previous group?!
  set->setAttribute(calib_root / "type", "CV8");
  set->setAttribute(calib_root / "projection", f, 2);
  set->setAttribute(calib_root / "projection_center", c, 2);
  set->setAttribute(calib_root / "opencv_distortion", dist);
  
  return true;
}

#ifdef CLIF_WITH_UCALIB
static void _calib_cam(Mat_<float> &proxy_m, Idx proxy_cam_idx, Idx res_idx, Mat_<float> &corr_line_m, Point2i proxy_size, Mat_<double> &rms, Mat_<double> &proj_rms, Mat_<float> &proj, Mat_<float> &extrinsics_m, Cam_Config cam_config, Calib_Config conf, int im_size[2])
{
  int img_count = proxy_m["views"];
  
  DistCorrLines dist_lines = DistCorrLines(0, 0, 0, cam_config.w, cam_config.h, 100.0, cam_config, conf, proxy_size);
  dist_lines.proxy_backwards.resize(img_count);

  for(auto pos : Idx_It_Dim(proxy_cam_idx, proxy_m, "views")) {
    int pos_int = pos["views"];
    dist_lines.proxy_backwards[pos_int].resize(proxy_size.y*proxy_size.x);
    for(int j=0;j<proxy_size.y;j++)
      for(int i=0;i<proxy_size.x;i++) {
        pos[1] = i;
        pos[2] = j;
        pos[0] = 0;
        dist_lines.proxy_backwards[pos_int][j*proxy_size.x+i].x = proxy_m(pos);
        pos[0] = 1;
        dist_lines.proxy_backwards[pos_int][j*proxy_size.x+i].y = proxy_m(pos);
      }
  }
  
  //Idx cam_pos({proxy_cam_idx.r(proxy_cam_idx.dim("y")+1,proxy_cam_idx.dim("views")-1)});
  
  Idx cam_pos({proxy_cam_idx.r("channels", "cams")});
  
  std::cout << "cam pos:" << cam_pos << std::endl;
  //std::cout << "proj size:" << proj << std::endl;
        
  rms(cam_pos) = dist_lines.proxy_fit_lines_global();
        
  GenCam gcam = GenCam(dist_lines.linefits, cv::Point2i(im_size[0],im_size[1]), cv::Point2i(proxy_m[1],proxy_m[2]));
  
  
  proj(0, cam_pos) = gcam.f.x;
  proj(1, cam_pos) = gcam.f.y;
  proj_rms(cam_pos) = gcam.rms;
  
  for(int j=0;j<proxy_size.y;j++)
    for(int i=0;i<proxy_size.x;i++)
      for(int l=0;l<4;l++) {
        corr_line_m({l, i, j, proxy_cam_idx.r("channels","cams")}) = dist_lines.linefits[j*proxy_size.x+i][l];
        //printf("%d %d %d %d %d = %f\n", l, i, j, proxy_cam_idx["channels"], proxy_cam_idx["cams"], dist_lines.linefits[j*proxy_size.x+i][l]);
      }
  
  for(int img=0;img<img_count;img++) {
    for(int i=0;i<6;i++) {
      extrinsics_m(i, proxy_cam_idx.r("channels", "cams"), img) = dist_lines.extrinsics[6*img+i];
      //std::cout << "set " << Idx({i, proxy_cam_idx.r("channels", "cams"), img}) << std::endl;
    }
  }
}
#endif

#ifdef CLIF_WITH_UCALIB
// Generic line error
struct LineZ3GenericCenterLineError {
  LineZ3GenericCenterLineError(double x, double y)
      : x_(x), y_(y) {}
      
  template <typename T>
  bool operator()(const T* const extr,
                  T* residuals) const {
                    
                    
    T p[3], w_p[3];
    w_p[0] = T(x_);
    w_p[1] = T(y_);
    w_p[2] = T(0);
    ceres::AngleAxisRotatePoint(extr, w_p, p);

    // camera[3,4,5] are the translation.
    p[0] += extr[3];
    p[1] += extr[4];
    p[2] += extr[5];
    
    //compare with projected pinhole camera ray
    //FIXME without factor the fit might ignore this!
    residuals[0] = p[0]*T(10000);
    residuals[1] = p[1]*T(10000);
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create(double x, double y) {
    return (new ceres::AutoDiffCostFunction<LineZ3GenericCenterLineError, 2, 6>(
                new LineZ3GenericCenterLineError(x, y)));
  }

  double x_,y_;
};

// Generic line error
struct LineZ3GenericCenterLineExtraError {
  LineZ3GenericCenterLineExtraError(double x, double y)
      : x_(x), y_(y) {}
      
  template <typename T>
  bool operator()(const T* const extr, const T* const extr_rel,
                  T* residuals) const {
                    
                    
    T p[3], p1[3], w_p[3];
    w_p[0] = T(x_);
    w_p[1] = T(y_);
    w_p[2] = T(0);
    ceres::AngleAxisRotatePoint(extr, w_p, p1);

    // camera[3,4,5] are the translation.
    p1[0] += extr[3];
    p1[1] += extr[4];
    p1[2] += extr[5];
    
    ceres::AngleAxisRotatePoint(extr_rel, p1, p);

    // camera[3,4,5] are the translation.
    p[0] += extr_rel[3];
    p[1] += extr_rel[4];
    p[2] += extr_rel[5];
    
    //compare with projected pinhole camera ray
    //FIXME without factor the fit might ignore this!
    residuals[0] = p[0]*T(10000);
    residuals[1] = p[1]*T(10000);
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create(double x, double y) {
    return (new ceres::AutoDiffCostFunction<LineZ3GenericCenterLineExtraError, 2, 6, 6>(
                new LineZ3GenericCenterLineExtraError(x, y)));
  }

  double x_,y_;
};

// Pinhole line error
struct LineZ3DirPinholeError {
  LineZ3DirPinholeError(double x, double y, double w = 1.0)
      : x_(x), y_(y), w_(w) {}
      
  template <typename T>
  bool operator()(const T* const extr, const T* const line,
                  T* residuals) const {
                    
                    
    T p[3], w_p[3];
    w_p[0] = T(x_);
    w_p[1] = T(y_);
    w_p[2] = T(0);
    ceres::AngleAxisRotatePoint(extr, w_p, p);

    // camera[3,4,5] are the translation.
    p[0] += extr[3];
    p[1] += extr[4];
    p[2] += extr[5];
    
    //compare with projected pinhole camera ray
    residuals[0] = (p[0] - p[2]*line[0])*T(w_);
    residuals[1] = (p[1] - p[2]*line[1])*T(w_);
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create(double x, double y, double w = 1.0) {
    return (new ceres::AutoDiffCostFunction<LineZ3DirPinholeError, 2, 6, 2>(
                new LineZ3DirPinholeError(x, y, w)));
  }

  double x_,y_,w_;
};

// Pinhole line error
struct LinExtrError {
  LinExtrError(double idx)
      : idx_(idx) {}
      
  template <typename T>
  bool operator()(const T* const dir, const T* const extr,
                  T* residuals) const {
                    
                    
    T c[3], w_c[3];
    c[0] = extr[3];
    c[1] = extr[4];
    c[2] = extr[5];
    ceres::AngleAxisRotatePoint(extr, c, w_c);
    
    //compare with projected pinhole camera ray
    residuals[0] = (w_c[0]-dir[0]*T(idx_))*T(enforce_cams_line);
    residuals[1] = (w_c[1]-dir[1]*T(idx_))*T(enforce_cams_line);
    residuals[2] = (w_c[2]-dir[2]*T(idx_))*T(enforce_cams_line);
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create(double idx) {
    return (new ceres::AutoDiffCostFunction<LinExtrError, 3, 3, 6>(
                new LinExtrError(idx)));
  }

  double idx_;
};

//center dir error (should be zero!)
struct LineZ3CenterDirError {
  LineZ3CenterDirError() {}
      
  template <typename T>
  bool operator()(const T* const line,
                  T* residuals) const {
    //compare with projected pinhole camera ray
    residuals[0] = line[0];
    residuals[1] = line[1];
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create() {
    return (new ceres::AutoDiffCostFunction<LineZ3CenterDirError, 2, 2>(
                new LineZ3CenterDirError()));
  }
};

//center dir and offset error (should be zero!)
struct LineZ3GenericCenterDirError {
  LineZ3GenericCenterDirError() {}
      
  template <typename T>
  bool operator()(const T* const line,
                  T* residuals) const {
    //compare with projected pinhole camera ray
    residuals[0] = line[0];
    residuals[1] = line[1];
    residuals[2] = line[2];
    residuals[3] = line[3];
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create() {
    return (new ceres::AutoDiffCostFunction<LineZ3GenericCenterDirError, 4, 4>(
                new LineZ3GenericCenterDirError()));
  }
};

// Pinhole line error
struct LineZ3DirPinholeExtraError {
  LineZ3DirPinholeExtraError(double x, double y, double w = 1.0)
      : x_(x), y_(y), w_(w) {}
      
  template <typename T>
  bool operator()(const T* const extr, const T* const extr_rel, const T* const line,
                  T* residuals) const {
                    
                    
    T p[3], p1[3], w_p[3];
    w_p[0] = T(x_);
    w_p[1] = T(y_);
    w_p[2] = T(0);
    ceres::AngleAxisRotatePoint(extr, w_p, p1);

    // camera[3,4,5] are the translation.
    p1[0] += extr[3];
    p1[1] += extr[4];
    p1[2] += extr[5];
    
    
    ceres::AngleAxisRotatePoint(extr_rel, p1, p);

    // camera[3,4,5] are the translation.
    p[0] += extr_rel[3];
    p[1] += extr_rel[4];
    p[2] += extr_rel[5];
    
    //compare with projected pinhole camera ray
    residuals[0] = (p[0] - p[2]*line[0])*T(w_);
    residuals[1] = (p[1] - p[2]*line[1])*T(w_);
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create(double x, double y, double w = 1.0) {
    return (new ceres::AutoDiffCostFunction<LineZ3DirPinholeExtraError, 2, 6, 6, 2>(
                new LineZ3DirPinholeExtraError(x, y, w)));
  }

  double x_,y_,w_;
};

struct RectProjDirError {
  RectProjDirError(cv::Point2d ip, double weight)
      : w(weight)
      {
        ix = ip.x;
        iy = ip.y;
      }

      
  template<typename T> 
  bool operator()(const T* const p, const T* const l,
                  T* residuals) const {
    T wx = l[0]; //dir
    T wy = l[1];
    T px = wx * p[0];    
    T py = wy * p[1];    
    residuals[0] = (T(ix)-px)*T(w);
    residuals[1] = (T(iy)-py)*T(w);
    residuals[2] = p[0]-p[1];
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create(cv::Point2d ip, double weight) {
    return (new ceres::AutoDiffCostFunction<RectProjDirError, 3, 2, 2>(
                new RectProjDirError(ip, weight)));
  }

  double ix,iy,w;
};

struct RectProjDirError4P {
  RectProjDirError4P(cv::Point2d ip, double weight)
      : w(weight)
      {
        ix = ip.x;
        iy = ip.y;
      }

      
  template<typename T> 
  bool operator()(const T* const p, const T* const l,
                  T* residuals) const {
    T wx = l[2]; //dir
    T wy = l[3];
    T px = wx * p[0];    
    T py = wy * p[1];    
    residuals[0] = (T(ix)-px)*T(w);
    residuals[1] = (T(iy)-py)*T(w);
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create(cv::Point2d ip, double weight) {
    return (new ceres::AutoDiffCostFunction<RectProjDirError4P, 2, 2, 4>(
                new RectProjDirError4P(ip, weight)));
  }

  double ix,iy,w;
};


// Generic line error
struct LineZ3GenericError {
  LineZ3GenericError(double x, double y)
      : x_(x), y_(y) {}
      
  template <typename T>
  bool operator()(const T* const extr, const T* const line,
                  T* residuals) const {
                    
                    
    T p[3], w_p[3];
    w_p[0] = T(x_);
    w_p[1] = T(y_);
    w_p[2] = T(0);
    ceres::AngleAxisRotatePoint(extr, w_p, p);

    // camera[3,4,5] are the translation.
    p[0] += extr[3];
    p[1] += extr[4];
    p[2] += extr[5];
    
    //compare with projected pinhole camera ray
    residuals[0] = (p[0] - (line[0] + p[2]*line[2]));
    residuals[1] = (p[1] - (line[1] + p[2]*line[3]));
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create(double x, double y) {
    return (new ceres::AutoDiffCostFunction<LineZ3GenericError, 2, 6, 4>(
                new LineZ3GenericError(x, y)));
  }

  double x_,y_;
};

// Generic line error
struct LineZ3GenericExtraError {
  LineZ3GenericExtraError(double x, double y)
      : x_(x), y_(y) {}
      
  template <typename T>
  bool operator()(const T* const extr, const T* const extr_rel, const T* const line,
                  T* residuals) const {
                    
                    
    T p[3], p1[3], w_p[3];
    w_p[0] = T(x_);
    w_p[1] = T(y_);
    w_p[2] = T(0);
    ceres::AngleAxisRotatePoint(extr, w_p, p1);

    // camera[3,4,5] are the translation.
    p1[0] += extr[3];
    p1[1] += extr[4];
    p1[2] += extr[5];
    
    ceres::AngleAxisRotatePoint(extr_rel, p1, p);

    // camera[3,4,5] are the translation.
    p[0] += extr_rel[3];
    p[1] += extr_rel[4];
    p[2] += extr_rel[5];
    
    //compare with projected pinhole camera ray
    residuals[0] = (p[0] - (line[0] + p[2]*line[2]));
    residuals[1] = (p[1] - (line[1] + p[2]*line[3]));
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create(double x, double y) {
    return (new ceres::AutoDiffCostFunction<LineZ3GenericExtraError, 2, 6, 6, 4>(
                new LineZ3GenericExtraError(x, y)));
  }

  double x_,y_;
};

// try to (lightly) enforece pinhole model
struct LineZ3GenericCenterError {
  LineZ3GenericCenterError() {}
      
  template <typename T>
  bool operator()(const T* const line,
                  T* residuals) const {
    residuals[0] = line[0]*T(center_constr_weight);
    residuals[1] = line[1]*T(center_constr_weight);
    
    return true;
  }

  // Factory to hide the construction of the CostFunction object from
  // the client code.
  static ceres::CostFunction* Create() {
    //should reduce this to 2 but ceres complains about the same pointer being added with four params
    return (new ceres::AutoDiffCostFunction<LineZ3GenericCenterError, 2, 4>(
                new LineZ3GenericCenterError()));
  }
};

double calc_line_pos_weight(cv::Point2i pos, cv::Point2i proxy_size, double r, double min_weight = non_center_rest_weigth)
{
  int pmax = std::max(proxy_size.x, proxy_size.y);
  
  double w = norm(Point2d((pos.x+0.5-proxy_size.x*0.5)/pmax,(pos.y+0.5-proxy_size.y*0.5)/pmax));
  
  return std::max(std::max(r-w, 0.0)*(1.0/r), min_weight);
}

static void _zline_problem_add_proj_error(ceres::Problem &problem, Mat_<double> &lines, cv::Point2i img_size, Mat_<double> &proj, double proj_constraint, double min_weight)
{
  double two_sigma_squared = 2.0*(img_size.x*img_size.x+img_size.y*img_size.y)*proj_center_size*proj_center_size;
  cv::Point2i proxy_size(lines["x"], lines["y"]);
      
  for(auto pos : Idx_It_Dims(lines, "x", "cams")) {        
    int x = pos["x"];
    int y = pos["y"];
    
    Point2d ip = Point2d((x+0.5-proxy_size.x*0.5)*img_size.x/proxy_size.x,(y+0.5-proxy_size.y*0.5)*img_size.y/proxy_size.y);
      
    double w = calc_line_pos_weight(cv::Point2i(x, y), proxy_size, center_constr_weight, min_weight);
    
    w *= proj_constraint;
    
    if (w == 0.0)
      continue;
    
    ceres::CostFunction* cost_function =
        RectProjDirError::Create(ip, w);
        
    problem.AddResidualBlock(cost_function,
                            NULL,
                            &proj(0,pos.r("channels","cams")),
                            &lines({2,pos.r("x","cams")}));
  }
}

static void _zline_problem_add_lin_views(ceres::Problem &problem, Mat_<double> &extrinsics_rel, double *dir)
{      
  for(auto pos : Idx_It_Dims(extrinsics_rel, "channels", "cams")) {
    if (!pos["cams"])
      continue;
    if (pos["cams"] > _calib_cams_limit)
      continue;
    
    ceres::CostFunction* cost_function =
        LinExtrError::Create(pos["cams"]);
        
    problem.AddResidualBlock(cost_function,
                            NULL,
                            dir,
                            &extrinsics_rel(pos));
  }
}



static void _zline_problem_add_proj_error_4P(ceres::Problem &problem, Mat_<double> &lines, cv::Point2i img_size, Mat_<double> &proj, double proj_constraint)
{
  double two_sigma_squared = 2.0*(img_size.x*img_size.x+img_size.y*img_size.y)*proj_center_size*proj_center_size;
  cv::Point2i proxy_size(lines["x"], lines["y"]);
      
  for(auto pos : Idx_It_Dims(lines, "x", "cams")) {        
    int x = pos["x"];
    int y = pos["y"];
    
    Point2d ip = Point2d((x+0.5-proxy_size.x*0.5)*img_size.x/proxy_size.x,(y+0.5-proxy_size.y*0.5)*img_size.y/proxy_size.y);
      
    double w = exp(-((ip.x*ip.x+ip.y*ip.y)/two_sigma_squared));
    //w_sum += w;
    w = sqrt(w);
    w *= proj_constraint;
    
    
    ceres::CostFunction* cost_function =
        RectProjDirError4P::Create(ip, w);
        
    problem.AddResidualBlock(cost_function,
                            NULL,
                            &proj(0,pos.r("channels","cams")),
                            &lines({0,pos.r("x","cams")}));
  }
}

static void _zline_problem_add_pinhole_lines(ceres::Problem &problem, cv::Point2i img_size, const Mat_<float>& proxy, Mat_<double> &extrinsics, Mat_<double> &extrinsics_rel, Mat_<double> &lines, ceres::LossFunction *loss = NULL, bool falloff = true, double min_weight = non_center_rest_weigth)
{
  cv::Point2i center(proxy["x"]/2, proxy["y"]/2);
  
  double two_sigma_squared = 2.0*(img_size.x*img_size.x+img_size.y*img_size.y)*extr_center_size*extr_center_size;
  cv::Point2i proxy_size(lines["x"], lines["y"]);
  
  //channels == 0 && cams == 0
  int last_view = 0;
  int ray_count = 0;
  for(auto ray : Idx_It_Dims(proxy, "x", "views")) {
    bool ref_cam = true;
    
    if (ray["cams"] > _calib_cams_limit)
      continue;
    if (ray["views"] > _calib_views_limit)
      continue;
    
    for(int i=proxy.dim("channels");i<=proxy.dim("cams");i++)
      if (ray[i])
        ref_cam = false;
      
    cv::Point2f p(proxy({0,ray.r("x",-1)}),
                  proxy({1,ray.r("x",-1)}));
    
    if (isnan(p.x) || isnan(p.y))
      continue;
    
    double w;
    
    /*if (falloff)
      w = calc_line_pos_weight(cv::Point2i(ray["x"], ray["y"]), proxy_size, extr_center_size, min_weight);
    else*/
      w = 1.0;
 
    if (ray["views"] != last_view) {
      printf("view %d: %d rays\n", last_view, ray_count);
      last_view = ray["views"];
      ray_count = 0;
    }
    
    //keep center looking straight (in local ref system)
    if (ray["y"] == center.y && ray["x"] == center.x) {
      ceres::CostFunction* cost_function =
      LineZ3CenterDirError::Create();
      problem.AddResidualBlock(cost_function, loss, 
                              &lines({2,ray.r("x","cams")}));
      ray_count++;
    }
    
    if (w > 0.0) {
      if (ref_cam) {
        //std::cout << "process: " << ray << p << "\n";
        //regular line error (in x/y world direction)
        ceres::CostFunction* cost_function = 
        LineZ3DirPinholeError::Create(p.x, p.y, w);
        problem.AddResidualBlock(cost_function,
                                NULL,
                                &extrinsics({0,ray["views"]}),
                                //use only direction part!
                                &lines({2,ray.r("x","cams")}));
      }
      else {
        ceres::CostFunction* cost_function = 
        LineZ3DirPinholeExtraError::Create(p.x, p.y, w);
        problem.AddResidualBlock(cost_function,
                                NULL,
                                &extrinsics({0,ray["views"]}),&extrinsics_rel({0,ray.r("channels","cams")}),
                                //use only direction part!
                                &lines({2,ray.r("x","cams")}));
      }
      ray_count++;
    }
  }
  printf("view %d: %d rays\n", last_view, ray_count);
}

class ApxMedian
{
public:
  ApxMedian(double step, double max)
  {
    _inv_step = 1.0/step;
    _lin_buckets = int(M_E*_inv_step)+1;
    _max_idx = _lin_buckets+log(max*100.0)*_inv_step;
    _buckets.resize(_max_idx);
    _max_idx--;
  };
  
  
  void add(double val)
  {
    val = abs(val)*100.0;
    
    if (val >= M_E)
      _buckets[min(int(_lin_buckets+log(val)*_inv_step), _max_idx)]++;
    else
      _buckets[val*_inv_step]++;
    
    _count++;
  };
  
  double median()
  {
    int sum = 0;
    for(int i=0;i<_max_idx;i++) {
      sum += _buckets[i];
      if (sum >= _count/2) {
        printf("bucket %d sum %d count/2: %d\n", i, sum, _count/2);
        if (i < _lin_buckets)
          return i/_inv_step*0.01;
        else
          return exp(i - _lin_buckets)*0.01;
      }
    }
  }
  
private:
  std::vector<int> _buckets;
  int _max_idx;
  int _lin_buckets;
  double _inv_step;
  int _count = 0;
};

static int _zline_problem_filter_pinhole_lines(const Mat_<float>& proxy, Mat_<double> &extrinsics, Mat_<double> &extrinsics_rel, Mat_<double> &lines, double threshold)
{
  double step = 0.0001;
  //ApxMedian med(step, 1.0);
  std::vector<float> scores;
  int count = 0, remain = 0;
  cv::Point2i center(proxy["x"]/2, proxy["y"]/2);
  
  //channels == 0 && cams == 0
  for(auto ray : Idx_It_Dims(proxy, "x", "views")) {
    bool ref_cam = true;
    
    for(int i=proxy.dim("channels");i<=proxy.dim("cams");i++)
      if (ray[i])
        ref_cam = false;
      
    cv::Point2f p(proxy({0,ray.r("x",-1)}),
                  proxy({1,ray.r("x",-1)}));
    
    if (isnan(p.x) || isnan(p.y))
      continue;
    
    double residuals[3];
    
    if (ref_cam) {
      LineZ3DirPinholeError err(p.x, p.y);
      err.operator()<double>(&extrinsics({0,ray["views"]}), &lines({2,ray.r("x","cams")}), residuals);
    }
    else {
      LineZ3DirPinholeExtraError err(p.x, p.y);
      err.operator()<double>(&extrinsics({0,ray["views"]}), &extrinsics_rel({0,ray.r("channels","cams")}), &lines({2,ray.r("x","cams")}), residuals);
    }
    
    double err = sqrt(residuals[0]*residuals[0]+residuals[1]*residuals[1]);
    scores.push_back(err);
    //med.add(err);
    /*if (err >= 0.1) {
      proxy({0,ray.r("x",-1)}) = std::numeric_limits<double>::quiet_NaN();
      proxy({1,ray.r("x",-1)}) = std::numeric_limits<double>::quiet_NaN();
    }*/
  }
  
  std::sort(scores.begin(), scores.end());
  
  double th = std::max(scores[scores.size()*99/100],scores[scores.size()]*2);// med.median()*4.0 + step;
  
  for(auto ray : Idx_It_Dims(proxy, "x", "views")) {
    bool ref_cam = true;
    
    for(int i=proxy.dim("channels");i<=proxy.dim("cams");i++)
      if (ray[i])
        ref_cam = false;
      
    cv::Point2f p(proxy({0,ray.r("x",-1)}),
                  proxy({1,ray.r("x",-1)}));
    
    if (isnan(p.x) || isnan(p.y))
      continue;
    
    double residuals[3];
    
    if (ref_cam) {
      LineZ3DirPinholeError err(p.x, p.y);
      err.operator()<double>(&extrinsics({0,ray["views"]}), &lines({2,ray.r("x","cams")}), residuals);
    }
    else {
      LineZ3DirPinholeExtraError err(p.x, p.y);
      err.operator()<double>(&extrinsics({0,ray["views"]}), &extrinsics_rel({0,ray.r("channels","cams")}), &lines({2,ray.r("x","cams")}), residuals);
    }
    
    double err = sqrt(residuals[0]*residuals[0]+residuals[1]*residuals[1]);
    //med.add(err);
    if (err >= th) {
      count++;
      proxy({0,ray.r("x",-1)}) = std::numeric_limits<double>::quiet_NaN();
      proxy({1,ray.r("x",-1)}) = std::numeric_limits<double>::quiet_NaN();
    }
    else
      remain++;
  }
  
  printf("median: %f\n", scores[scores.size()/2]);
  printf("removed %d data points, %d remain\n", count, remain);
  return count;
}

static void _zline_problem_add_generic_lines(ceres::Problem &problem, const Mat_<float>& proxy, Mat_<double> &extrinsics, Mat_<double> &extrinsics_rel, Mat_<double> &lines, bool reproj_error_calc_only = false)
{
  cv::Point2i center(proxy["x"]/2, proxy["y"]/2);
  
  //channels == 0 && cams == 0
  for(auto ray : Idx_It_Dims(proxy, "x", "views")) {
    bool ref_cam = true;
    
    for(int i=proxy.dim("channels");i<=proxy.dim("cams");i++)
      if (ray[i])
        ref_cam = false;
      
    cv::Point2f p(proxy({0,ray.r("x",-1)}),
                  proxy({1,ray.r("x",-1)}));
    
    if (isnan(p.x) || isnan(p.y))
      continue;
    
    //keep center looking straight
    if (ray["y"] == center.y && ray["x"] == center.x && !reproj_error_calc_only) {
      ceres::CostFunction* cost_function =
      LineZ3GenericCenterDirError::Create();
      problem.AddResidualBlock(cost_function, NULL, 
                              &lines({0,ray.r("x","cams")}));
    }
  
    if (ref_cam) {
      //std::cout << "process: " << ray << p << "\n";
      

      //regular line error (in x/y world direction)
      ceres::CostFunction* cost_function = 
      LineZ3GenericError::Create(p.x, p.y);
      problem.AddResidualBlock(cost_function,
                              NULL,
                              &extrinsics({0,ray["views"]}),
                              &lines({0,ray.r("x","cams")}));
    }
    else {
        ceres::CostFunction* cost_function = 
        LineZ3GenericExtraError::Create(p.x, p.y);
        problem.AddResidualBlock(cost_function,
                                NULL,
                                &extrinsics({0,ray["views"]}),
                                &extrinsics_rel({0,ray.r("channels","cams")}),
                                &lines({0,ray.r("x","cams")}));
    }
  }
}

static void _zline_problem_add_center_errors(ceres::Problem &problem, Mat_<double> &lines)
{
  for(auto pos : Idx_It_Dims(lines, "x", "cams")) {
    ceres::CostFunction* cost_function = LineZ3GenericCenterError::Create();
    problem.AddResidualBlock(cost_function, NULL,
                            &lines({0,pos.r("x","cams")}));
  }
}
#endif

Mat_<double> proj;

#ifdef CLIF_WITH_LIBIGL_VIEWER
void update_cams_mesh(Mesh &cams, Mat_<double> extrinsics, Mat_<double> extrinsics_rel, Mat_<double> lines)
{
  cams = mesh_cam().scale(20);
  
  for(auto pos : Idx_It_Dims(extrinsics_rel,"channels","cams")) {
      //cv::Vec3b col(0,0,0);
      //col[ch%3]=255;
      Eigen::Vector3d trans(extrinsics_rel(3,pos.r("channels","cams")),extrinsics_rel(4,pos.r("channels","cams")),extrinsics_rel(5,pos.r("channels","cams")));
      Eigen::Vector3d rot(extrinsics_rel(0,pos.r("channels","cams")),extrinsics_rel(1,pos.r("channels","cams")),extrinsics_rel(2,pos.r("channels","cams")));
      
      //FIXME must exactly invert those operations?
      Mesh cam = mesh_cam().scale(20);
      cam -= trans;
      cam.rotate(-rot);
      cams.merge(cam);
      //cam_writer.add(cam_left,col);
      //printf("extr:\n");
      //std::cout << -rot << "\n trans:\n" << -trans << std::endl;
      //printf("%.3f ", trans.norm());
      
      Mesh line_mesh;
      
      for(auto line_pos : Idx_It_Dims(lines,"x","y")) {
        if (line_pos["x"] % 4 != 0)
          continue;
        if (line_pos["y"] % 4 != 0)
          continue;
        Eigen::Vector3d origin(lines({0,line_pos.r("x","y"),pos.r("channels","cams")}),lines({1,line_pos.r("x","y"),pos.r("channels","cams")}),0.0);
        Eigen::Vector3d dir(lines({2,line_pos.r("x","y"),pos.r("channels","cams")}),lines({3,line_pos.r("x","y"),pos.r("channels","cams")}),-1.0);
        
      
        Mesh line = mesh_line(origin,origin+dir*1000.0);
        
        line_mesh.merge(line);
      }
      
      line_mesh -= trans;
      line_mesh.rotate(-rot);
      
      cams.merge(line_mesh);
      //viewer.data.add_edges(P1,P2,Eigen::RowVector3d(r,g,b);
    }
  //printf("\n");
  
  for(auto pos : Idx_It_Dim(extrinsics, "views")) {
      Eigen::Vector3d trans(extrinsics(3,pos["views"]),extrinsics(4,pos["views"]),extrinsics(5,pos["views"]));
      Eigen::Vector3d rot(extrinsics(0,pos["views"]),extrinsics(1,pos["views"]),extrinsics(2,pos["views"]));
      
      Mesh plane = mesh_plane().scale(1000);
      
      //printf("trans %d %fx%fx%f\n", pos["views"], trans(0), trans(1), trans(2));
      
      plane -= trans;
      plane.rotate(-rot);
      
      cams.merge(plane);
  }
  
  printf("proj %f %f\n", proj(0, 0), proj(1,0));
}

//globals for viewer
igl::viewer::Viewer *_viewer;
Mat_<double> *extr_ptr = NULL;
Mat_<double> *extr_rel_ptr = NULL;
Mat_<double> *lines_ptr = NULL;
Mesh mesh;

//this is called in main thread!
bool callback_pre_draw(igl::viewer::Viewer& viewer)
{ 
  if (extr_ptr) {
    viewer.data.clear();
    update_cams_mesh(mesh, *extr_ptr, *extr_rel_ptr, *lines_ptr);
    viewer.data.set_mesh(mesh.V, mesh.F);
  }
  
  viewer.callback_pre_draw = nullptr;
  return false;
}

#ifdef CLIF_WITH_UCALIB
class ceres_iter_callback : public ceres::IterationCallback {
 public:
  explicit ceres_iter_callback() {};

  ~ceres_iter_callback() {}

  virtual ceres::CallbackReturnType operator()(const ceres::IterationSummary& summary) {
    _viewer->callback_pre_draw = callback_pre_draw;
    glfwPostEmptyEvent();
    return ceres::SOLVER_CONTINUE;
  }
};
#endif


void run_viewer(Mesh *mesh)
{
  if (!_viewer)
    _viewer = new igl::viewer::Viewer;
  
  _viewer->core.set_rotation_type(igl::viewer::ViewerCore::RotationType::ROTATION_TYPE_TRACKBALL);
  _viewer->data.set_mesh(mesh->V, mesh->F);
  
  _viewer->launch();
}

#endif

#ifdef CLIF_WITH_UCALIB

double solve_pinhole(const ceres::Solver::Options &options, const Mat_<float>& proxy, Mat_<double> &lines, Point2i img_size, Mat_<double> &extrinsics, Mat_<double> &extrinsics_rel, Mat_<double> proj, double proj_weight, double min_weight = non_center_rest_weigth)
{
  ceres::Solver::Summary summary;
  ceres::Problem problem;
  double dir[3] = {0,0,0};
  
  _zline_problem_add_pinhole_lines(problem, img_size, proxy, extrinsics, extrinsics_rel, lines, NULL, true, min_weight);
  if (proj_weight > 0.0)
    _zline_problem_add_proj_error(problem, lines, img_size, proj, proj_weight, 0.0);
  //_zline_problem_add_lin_views(problem, extrinsics_rel, dir);
  
  printf("solving pinhole problem (proj w = %f...\n", proj_weight);
  ceres::Solve(options, &problem, &summary);
  std::cout << summary.FullReport() << "\n";
  printf("\npinhole rms ~%fmm\n", 2.0*sqrt(summary.final_cost/problem.NumResiduals()));
  
  return 2.0*sqrt(summary.final_cost/problem.NumResiduals());
}

int filter_pinhole(const Mat_<float>& proxy, Mat_<double> &lines, Point2i img_size, Mat_<double> &extrinsics, Mat_<double> &extrinsics_rel, Mat_<double> proj, double threshold)
{
  return _zline_problem_filter_pinhole_lines(proxy, extrinsics, extrinsics_rel, lines, threshold);
}

/*
 * optimize together:
 *  - per image camera movement (world rotation and translation)
 *  - individual lines (which do not need to converge in an optical center
 */
//proxy.names({"point","x","y","channels","cams","views"});
//lines.names({"line","x","y","channels","cams"})
double fit_cams_lines_multi(const Mat_<float>& proxy, Mat_<double> &lines, Point2i img_size, Mat_<double> &extrinsics, Mat_<double> &extrinsics_rel)
{
  ceres::Solver::Options options;
  options.max_num_iterations = 5000;
  //options.function_tolerance = 1e-12;
  //options.parameter_tolerance = 1e-12;
  options.minimizer_progress_to_stdout = true;
  //options.linear_solver_type = ceres::SPARSE_NORMAL_CHOLESKY;
  
  lines.create({IR(4, "line"), proxy.r("x", "cams")});
  extrinsics.create({IR(6, "extrinsics"), IR(proxy["views"], "views")});
  extrinsics_rel.create({IR(6, "extrinsics"), proxy.r("channels","cams")});
  
  //for threading!
  //options.linear_solver_type = ceres::DENSE_SCHUR;
  options.linear_solver_type = ceres::SPARSE_SCHUR;
  
  options.num_threads = 8;
  options.num_linear_solver_threads = 8;

#ifdef CLIF_WITH_LIBIGL_VIEWER
  ceres_iter_callback callback;
  
  options.callbacks.push_back(&callback);
  options.update_state_every_iteration = true;
#endif
  
  Mat_<double> r({proxy.r("channels","cams")});
  Mat_<double> m({2, proxy.r("channels","cams")});
  //Mat_<double> proj({2,proxy.r("channels","cams")});
  proj.create({2,proxy.r("channels","cams")});
  
  for(auto pos : Idx_It_Dims(r, 0, -1))
    r(pos) = 0;
  for(auto pos : Idx_It_Dims(m, 0, -1))
    m(pos) = 0;
  for(auto pos : Idx_It_Dims(proj, 0, -1))
    proj(pos) = 10000;
  
  for(auto cam_pos : Idx_It_Dim(extrinsics, "views")) {
    for(int i=0;i<5;i++)
      extrinsics({i, cam_pos["views"]}) = 0;
    extrinsics({5, cam_pos["views"]}) = 1000;
    //extrinsics({2, cam_pos["views"]}) = M_PI*0.5;
  }

  for(auto pos : Idx_It_Dims(extrinsics_rel, 0, -1))
    extrinsics_rel(pos) = 0;
  
  //for(auto pos : Idx_It_Dims(extrinsics_rel, 1, -1))
    //extrinsics_rel({3,pos.r(1,-1)}) = -50*pos["cams"];
  
  for(auto line_pos : Idx_It_Dims(lines, 0, -1))
    lines(line_pos) = 0;
  
  //mesh display
#ifdef CLIF_WITH_LIBIGL_VIEWER 
  extr_ptr = &extrinsics;
  extr_rel_ptr = &extrinsics_rel;
  lines_ptr = &lines;
  
  update_cams_mesh(mesh, extrinsics, extrinsics_rel, lines);
  
  std::thread viewer_thread(run_viewer, &mesh);
  
#endif
  
  /*ceres::Solver::Summary summary;
  ceres::Problem problem;
  
  _zline_problem_add_pinhole_lines(problem, proxy, extrinsics, extrinsics_rel, lines);
  _zline_problem_add_proj_error(problem, lines, img_size, proj, strong_proj_constr_weight);
  
  printf("solving pinhole problem (strong projection)...\n");
  ceres::Solve(options, &problem, &summary);
  std::cout << summary.FullReport() << "\n";
  printf("\npinhole rms ~%fmm\n", 2.0*sqrt(summary.final_cost/problem.NumResiduals()));*/
  
  /*int filtered = 1;
  while (filtered) {
    solve_pinhole(options, proxy, lines, img_size, extrinsics, extrinsics_rel, proj, strong_proj_constr_weight);
    filtered = filter_pinhole(proxy, lines, img_size, extrinsics, extrinsics_rel, proj, 0.2);
  }
  filtered = 1;
  while (filtered) {
    solve_pinhole(options, proxy, lines, img_size, extrinsics, extrinsics_rel, proj, proj_constr_weight);
    filtered = filter_pinhole(proxy, lines, img_size, extrinsics, extrinsics_rel, proj, 0.2);
  }
  filtered = 1;
  while (filtered) {
    solve_pinhole(options, proxy, lines, img_size, extrinsics, extrinsics_rel, proj, 0.0);
    filtered = filter_pinhole(proxy, lines, img_size, extrinsics, extrinsics_rel, proj, 0.2);
  }*/
  
  
  /*options.max_num_iterations = 200;
  _calib_cams_limit = 0;
  for(_calib_views_limit=0;_calib_views_limit<proxy["views"];_calib_views_limit=std::min(_calib_views_limit*2+1,proxy["views"]))
    solve_pinhole(options, proxy, lines, img_size, extrinsics, extrinsics_rel, proj, strong_proj_constr_weight, non_center_rest_weigth);
  options.max_num_iterations = 5000;*/
  
  for(_calib_cams_limit=0;_calib_cams_limit<proxy["cams"];_calib_cams_limit=std::min(_calib_cams_limit*4+1,proxy["cams"])) {
    solve_pinhole(options, proxy, lines, img_size, extrinsics, extrinsics_rel, proj, strong_proj_constr_weight, non_center_rest_weigth);
  }
  /*int filtered = 1;
  while (filtered) {
    solve_pinhole(options, proxy, lines, img_size, extrinsics, extrinsics_rel, proj, strong_proj_constr_weight, non_center_rest_weigth);
    filtered = filter_pinhole(proxy, lines, img_size, extrinsics, extrinsics_rel, proj, 0.2);
  }*/
  solve_pinhole(options, proxy, lines, img_size, extrinsics, extrinsics_rel, proj, strong_proj_constr_weight, non_center_rest_weigth);
  //solve_pinhole(options, proxy, lines, img_size, extrinsics, extrinsics_rel, proj, 1e-4, 0.0);
  
  
  //_zline_problem_eval_pinhole_lines(problem, proxy, extrinsics, extrinsics_rel, lines);
  
  /*ceres::Problem problem2;
  
  _zline_problem_add_pinhole_lines(problem2, proxy, extrinsics, extrinsics_rel, lines);
  _zline_problem_add_proj_error(problem2, lines, img_size, proj, proj_constr_weight);
  
  printf("solving pinhole problem (weak projection)...\n");
  ceres::Solve(options, &problem2, &summary);
  std::cout << summary.FullReport() << "\n";
  printf("\npinhole rms ~%fmm\n", 2.0*sqrt(summary.final_cost/problem2.NumResiduals()));
  
  printf("proj %fx%f, move %fx%f\n", proj[0], proj[1], m[0], m[1]);
  
  std::cout << summary.FullReport() << "\n";
  
  printf("rms: %f\n",2.0*sqrt(summary.final_cost/problem.NumResiduals()));
  
  //return 2.0*sqrt(summary.final_cost/problem.NumResiduals());
  
  ///////////////////////////////////////////////
  //full problem
  ///////////////////////////////////////////////
  options.max_num_iterations = 5000;
  options.function_tolerance = 1e-12;
  options.parameter_tolerance = 1e-12;
  
  ceres::Problem problem_full;
  
  _zline_problem_add_generic_lines(problem_full, proxy, extrinsics, extrinsics_rel, lines);
  _zline_problem_add_center_errors(problem_full, lines);
  _zline_problem_add_proj_error_4P(problem_full, lines, img_size, proj, proj_constr_weight);
  
  printf("solving constrained generic problem...\n");
  ceres::Solve(options, &problem_full, &summary);
  std::cout << summary.FullReport() << "\n";
  printf("\nunconstrained rms ~%fmm\n", 2.0*sqrt(summary.final_cost/problem_full.NumResiduals()));
  
  options.max_num_iterations = 0;
  ceres::Problem problem_reproj;
  _zline_problem_add_generic_lines(problem_reproj, proxy, extrinsics, extrinsics_rel, lines, true);
  
  ceres::Solve(options, &problem_reproj, &summary);
  
  printf("\nunconstrained rms ~%fmm\n", 2.0*sqrt(summary.final_cost/problem_reproj.NumResiduals()));*/

#ifdef CLIF_WITH_LIBIGL_VIEWER
  //glfwSetWindowShouldClose(_viewer->window, 1); 
  viewer_thread.join();
#endif
  printf("finished\n");
}
#endif

//FIXME repair!
bool ucalib_calibrate(Dataset *set, cpath proxy, cpath calib)
#ifdef CLIF_WITH_UCALIB
{
  cpath proxy_root, calib_root, extr_root;
  if (!set->deriveGroup("calibration/proxy", proxy, "calibration/intrinsics", calib, proxy_root, calib_root))
    abort();
  
  
  if (!set->deriveGroup("calibration/proxy", proxy, "calibration/extrinsics", calib, proxy_root, extr_root))
    abort();
  
  int im_size[2];
    
  vector<vector<Point2f>> ipoints;
  vector<vector<Point3f>> wpoints;
  
  Mat_<float> proxy_m;
  Mat_<float> corr_line_m;
  
  set->get(proxy_root/"source/img_size", im_size, 2);
  
  Cam_Config cam_config = { 0.0065, 12.0, 300.0, -1, -1 };
  Calib_Config conf = { true, 190, 420 };

  cam_config.w = im_size[0];
  cam_config.h = im_size[1];
    
  Datastore *proxy_store = set->store(proxy_root/"proxy");
  proxy_store->read(proxy_m);
  
  if (proxy_m.size() == 5) {
    printf("FIXME implement 5D calib!");
    return true;
  }
  else { //proxy large than 5
    //last dim is views from same camera!
    
    //FIXME hardcoded for now
    proxy_m.names({"point","x","y","channels","cams","views"});
    
    Mat_<double> lines;
    Mat_<double> extrinsics;
    Mat_<double> extrinsics_rel;
    
    //FIXME hack!
    /*std::cout << proxy_m << "\n";
    for(auto pos : Idx_It_Dims(proxy_m, 0,-1)) {
      if (!isnan(proxy_m(pos))) {
        if (pos["cams"] == 0 && pos["views"] == 0)
          printf("%dx%d %d: %f -> %f\n",pos["x"],pos["y"],pos["point"],proxy_m(pos),proxy_m(pos)*5);
        proxy_m(pos) *= 5;
      }
    }*/
    
    double rms = fit_cams_lines_multi(proxy_m, lines, cv::Point2i(im_size[0],im_size[1]), extrinsics, extrinsics_rel);
    
    Datastore *line_store = set->addStore(calib_root/"lines");
    line_store->write(lines);
    
    
    set->setAttribute(calib_root/"type", "UCALIB");
    set->setAttribute(calib_root/"extrinsics_views", extrinsics);
    set->setAttribute(calib_root/"extrinsics_cams", extrinsics_rel);
    set->setAttribute(calib_root/"rms", rms);
    //set->setAttribute(calib_root/"projection", proj);
    std::vector<double> proj_hack(2);
    proj_hack[0] = proj(0);
    proj_hack[1] = proj(0);
    set->setAttribute(calib_root/"projection", proj_hack);
    
    cv::Point3f step(extrinsics_rel(3,0,extrinsics_rel["cams"]-1),extrinsics_rel(4,0,extrinsics_rel["cams"]-1),extrinsics_rel(5,0,extrinsics_rel["cams"]-1));
    std::cout << step << "\n";
    std::vector<double> step_vec = {norm(step)/extrinsics_rel["cams"],0,0};
    set->setAttribute(extr_root/"line_step", step_vec);
    printf("baseline: %f\n", step_vec[0]);
    set->setAttribute(extr_root/"type", "LINE");

    printf("finished ucalib calibration!\n");
    return true;

    /*int step_count = 0;
    cv::Matx31f avg_step(0,0,0);
    
    printf("start ucalib calibration!\n");
    
    int views_dim = proxy_m.size()-1;
    
    Idx corr_size {IR(4, "line"), proxy_m.r("x", "cams")};
    Idx cams_size {proxy_m.r("channels","cams")};
    std::cout << "cams_size: " << cams_size << "\n";
    
    corr_line_m.create(corr_size);
    std::cout << "corr_line_m: " << corr_line_m << "\n";
    
    Point2i proxy_size(proxy_m[1],proxy_m[2]);
    
    Mat_<double> rms_m(cams_size);
    Mat_<double> proj_rms_m(cams_size);
    
    Mat_<float> proj_m({IR(2, "projection"), cams_size});
    
    //TODO first place to need view dimension - rework to make views optional?
    Mat_<float> extrinsics_m({IR(6, "extrinsics"), proxy_m.r("channels","views")});
    
    for(auto pos : Idx_It_Dims(proxy_m, "channels","cams")) {
      std::cout << "process cam:" << pos << std::endl;
      Idx res_idx(cams_size.size());
      for(int i=0;i<res_idx.size();i++)
	res_idx[i] = pos[i+3];
      //FIXME should solve all cams at the same time!
      _calib_cam(proxy_m, pos, res_idx, corr_line_m, proxy_size, rms_m, proj_rms_m, proj_m, extrinsics_m, cam_config,conf, im_size);
      
      cv::Matx31f step;
      if (pos["cams"] > 0) {
	for(int i=3;i<6;i++) {
	  step(i-3) = extrinsics_m({i, pos.r("channels","views")}) - extrinsics_m({i, pos["channels"], pos["cams"]-1, pos["views"]});
	  printf("%f = %f - %f\n", step(i-3), extrinsics_m({i, pos.r("channels","views")}), extrinsics_m({i, pos["channels"], pos["cams"]-1, pos["views"]}));
	}
	printf("step: %fx%fx%f\n", step(0), step(1), step(2));
	avg_step += step;
	step_count++;
      }
      
      printf("\n");
    }
    
    Datastore *line_store = set->addStore(calib_root/"lines");
    line_store->write(corr_line_m);
    
    set->setAttribute(calib_root/"type", "UCALIB");
    set->setAttribute(calib_root/"rms", rms_m);
    set->setAttribute(calib_root/"projection_rms", proj_rms_m);
    set->setAttribute(calib_root/"projection", proj_m);
    set->setAttribute(calib_root/"extrinsics", extrinsics_m);
    
    avg_step *= 1.0/step_count;
    
    printf("average step: %fx%fx%f\n", avg_step(0), avg_step(1), avg_step(2));
    
    
    
    printf("finished ucalib calibration!\n");
    return true;*/
  }
}
#else
{
  //FIXME report error
  printf("ERROR calibration not available - clif compiled without ucalib!\n");
  return false;
}
#endif
  
//FIXME repair!
bool ucalib_calibrate_1(Dataset *set, cpath proxy, cpath calib)
#ifdef CLIF_WITH_UCALIB
{
  cpath proxy_root, calib_root;
  if (!set->deriveGroup("calibration/proxy", proxy, "calibration/intrinsics", calib, proxy_root, calib_root))
    abort();
  
  int im_size[2];
    
  vector<vector<Point2f>> ipoints;
  vector<vector<Point3f>> wpoints;
  
  Mat_<float> proxy_m;
  Mat_<float> corr_line_m;
  
  set->get(proxy_root/"source/img_size", im_size, 2);
  
  Cam_Config cam_config = { 0.0065, 12.0, 300.0, -1, -1 };
  Calib_Config conf = { true, 190, 420 };

  cam_config.w = im_size[0];
  cam_config.h = im_size[1];
    
  Datastore *proxy_store = set->store(proxy_root/"proxy");
  proxy_store->read(proxy_m);
  
  if (proxy_m.size() == 5) {
    corr_line_m.create({4, proxy_m[1],proxy_m[2], proxy_m[3]});
    
    Point2i proxy_size(proxy_m[1],proxy_m[2]);
    
    std::vector<double> rms(proxy_m[3]);
    std::vector<double> proj_rms(proxy_m[3]);
    Mat_<float> proj(Idx{2, proxy_m[3]});
    Mat_<float> extrinsics_m(Idx{6, proxy_m[3], proxy_m[4]});
    
    for(int color=0;color<proxy_m[3];color++) {
      DistCorrLines dist_lines = DistCorrLines(0, 0, 0, cam_config.w, cam_config.h, 100.0, cam_config, conf, proxy_size);
      dist_lines.proxy_backwards.resize(proxy_m[4]);
      
      Idx pos(proxy_store->dims());
      assert(proxy_store->dims() == 5);
      
      for(int img_n=0;img_n<proxy_m[4];img_n++) {
	dist_lines.proxy_backwards[img_n].resize(proxy_size.y*proxy_size.x);
	for(int j=0;j<proxy_size.y;j++)
	  for(int i=0;i<proxy_size.x;i++) {
	    dist_lines.proxy_backwards[img_n][j*proxy_size.x+i].x = proxy_m(0, i, j, color, img_n);
	    dist_lines.proxy_backwards[img_n][j*proxy_size.x+i].y = proxy_m(1, i, j, color, img_n);
	  }
      }
      
      rms[color] = dist_lines.proxy_fit_lines_global();
      char buf[64];
      sprintf(buf, "center%02d", color);
      dist_lines.Draw(buf);
      
      GenCam gcam = GenCam(dist_lines.linefits, cv::Point2i(im_size[0],im_size[1]), cv::Point2i(proxy_m[1],proxy_m[2]));
      proj(0, color) = gcam.f.x;
      proj(1, color) = gcam.f.y;
      proj_rms[color] = gcam.rms;
      
      for(int j=0;j<proxy_size.y;j++)
	for(int i=0;i<proxy_size.x;i++) {
	  corr_line_m(0, i, j, color) = dist_lines.linefits[j*proxy_size.x+i][0];
	  corr_line_m(1, i, j, color) = dist_lines.linefits[j*proxy_size.x+i][1];
	  corr_line_m(2, i, j, color) = dist_lines.linefits[j*proxy_size.x+i][2];
	  corr_line_m(3, i, j, color) = dist_lines.linefits[j*proxy_size.x+i][3];
	}
	
      for(int img_n=0;img_n<proxy_m[4];img_n++)
	for(int i=0;i<6;i++)
	  extrinsics_m(i, color, img_n) = dist_lines.extrinsics[6*img_n+i];
    }
    
    Datastore *line_store = set->addStore(calib_root/"lines");
    line_store->write(corr_line_m);
    
    set->setAttribute(calib_root/"type", "UCALIB");
    set->setAttribute(calib_root/"rms", rms);
    set->setAttribute(calib_root/"projection_rms", proj_rms);
    set->setAttribute(calib_root/"projection", proj);
    set->setAttribute(calib_root/"extrinsics", extrinsics_m);
    
    return true;
  }
  else { //proxy large than 5
    //last dim is views from same camera!
    
    //FIXME hardcoded for now
    proxy_m.names({"point","x","y","channels","cams","views"});
    
    int step_count = 0;
    cv::Matx31f avg_step(0,0,0);
    
    printf("start ucalib calibration!\n");
    
    int views_dim = proxy_m.size()-1;
    
    Idx corr_size {IR(4, "line"), proxy_m.r("x", "cams")};
    Idx cams_size {proxy_m.r("channels","cams")};
    std::cout << "cams_size: " << cams_size << "\n";
    
    corr_line_m.create(corr_size);
    std::cout << "corr_line_m: " << corr_line_m << "\n";
    
    Point2i proxy_size(proxy_m[1],proxy_m[2]);
    
    Mat_<double> rms_m(cams_size);
    Mat_<double> proj_rms_m(cams_size);
    
    Mat_<float> proj_m({IR(2, "projection"), cams_size});
    
    Idx extrinsics_m_size(cams_size.size()+2);
    extrinsics_m_size[0] = 6;
    for(int i=0;i<cams_size.size();i++)
      extrinsics_m_size[i+1] = cams_size[i];
    extrinsics_m_size[extrinsics_m_size.size()-1] = proxy_m[views_dim];
    
    //TODO first place to need view dimension - rework to make views optional?
    Mat_<float> extrinsics_m({IR(6, "extrinsics"), proxy_m.r("channels","views")});
    
    for(auto pos : Idx_It_Dims(proxy_m, "channels","cams")) {
      std::cout << "process cam:" << pos << std::endl;
      Idx res_idx(cams_size.size());
      for(int i=0;i<res_idx.size();i++)
	res_idx[i] = pos[i+3];
      //FIXME should solve all cams at the same time!
      _calib_cam(proxy_m, pos, res_idx, corr_line_m, proxy_size, rms_m, proj_rms_m, proj_m, extrinsics_m, cam_config,conf, im_size);
      
      cv::Matx31f step;
      if (pos["cams"] > 0) {
	for(int i=3;i<6;i++) {
	  step(i-3) = extrinsics_m({i, pos.r("channels","views")}) - extrinsics_m({i, pos["channels"], pos["cams"]-1, pos["views"]});
	  printf("%f = %f - %f\n", step(i-3), extrinsics_m({i, pos.r("channels","views")}), extrinsics_m({i, pos["channels"], pos["cams"]-1, pos["views"]}));
	}
	//printf("step: %fx%fx%f\n", step(0), step(1), step(2));
	avg_step += step;
	step_count++;
      }
      
      //printf("\n");
    }
    
    Datastore *line_store = set->addStore(calib_root/"lines");
    line_store->write(corr_line_m);
    
    set->setAttribute(calib_root/"type", "UCALIB");
    set->setAttribute(calib_root/"rms", rms_m);
    set->setAttribute(calib_root/"projection_rms", proj_rms_m);
    set->setAttribute(calib_root/"projection", proj_m);
    set->setAttribute(calib_root/"extrinsics", extrinsics_m);
    
    avg_step *= 1.0/step_count;
    
    printf("average step: %fx%fx%f\n", avg_step(0), avg_step(1), avg_step(2));
    
    
    
    printf("finished ucalib calibration!\n");
    return true;
  }
}
#else
{
  //FIXME report error
  return false;
}
#endif
  
bool generate_proxy_loess(Dataset *set, int proxy_w, int proxy_h , cpath map, cpath proxy)
#ifdef CLIF_WITH_UCALIB
{
  cpath map_root, proxy_root;
  if (!set->deriveGroup("calibration/mapping", map, "calibration/proxy", proxy, map_root, proxy_root))
    abort();
  
  cv::Mat cam;
  vector<double> dist;
  vector<cv::Mat> rvecs;
  vector<cv::Mat> tvecs;
  int im_size[2];
  
  Mat_<std::vector<Point2f>> wpoints_m;
  Mat_<std::vector<Point2f>> ipoints_m;
  
  Mat_<float> proxy_m;
  
  //2-d (color, imgs)
  Attribute *w_a = set->get(map_root/"world_points");
  Attribute *i_a = set->get(map_root/"img_points");
  set->get(map_root/"img_size", im_size, 2);
  
  
  //FIXME error handling
  if (!w_a || !i_a)
    abort();
  
  w_a->get(wpoints_m);
  i_a->get(ipoints_m);
  
  Idx proxy_size(wpoints_m.size()+3);
  proxy_size[0] = 2; // 2d points
  proxy_size[1] = proxy_w;
  proxy_size[2] = proxy_h;
  for(int i=3;i<proxy_size.size();i++)
    proxy_size[i] = wpoints_m[i-3];
  
  proxy_m.create(proxy_size);
  
  Idx map_pos(wpoints_m.size());
  
  Cam_Config cam_config = { 0.0065, 12.0, 300.0, im_size[0], im_size[1] };
  Calib_Config conf = { true, 190, 420 };
  
  for(int color=0;color<wpoints_m[0];color++) {
    
    map_pos = Idx(wpoints_m.size());
    map_pos[0] = color;
    
    for(;map_pos<wpoints_m;map_pos.step(0, wpoints_m)) {
      if (!wpoints_m(map_pos).size())
        continue;
      
      vector<Point2f> ipoints(ipoints_m(map_pos));
      vector<Point3f> wpoints(ipoints_m(map_pos).size());
      
      for(int j=0;j<wpoints_m(map_pos).size();j++)
        wpoints[j] = Point3f(wpoints_m(map_pos)[j].x,wpoints_m(map_pos)[j].y,0);
      
      vector<vector<Point2f>> ipoints_ar(1);
      vector<vector<Point3f>> wpoints_ar(1);
      
      ipoints_ar[0] = ipoints;
      wpoints_ar[0] = wpoints;
      
      DistCorrLines dist_lines = DistCorrLines(0, 0, 0, cam_config.w, cam_config.h, 100.0, cam_config, conf, Point2i(proxy_w,proxy_h));
      dist_lines.add(ipoints_ar, wpoints_ar, 20.0);
      
      std::cout << "fit " << map_pos << std::endl;
      dist_lines.proxy_backwards_poly_generate();
      
      Idx proxy_pos(wpoints_m.size()+3);
      for(int i=0;i<map_pos.size();i++)
        proxy_pos[i+3] = map_pos[i];
      
      for(int j=0;j<proxy_h;j++)
        for(int i=0;i<proxy_w;i++) {
          proxy_pos[1] = i;
          proxy_pos[2] = j;
          proxy_pos[0] = 0;
          proxy_m(proxy_pos) = dist_lines.proxy_backwards[0][j*proxy_w+i].x;
          proxy_pos[0] = 1;
          proxy_m(proxy_pos) = dist_lines.proxy_backwards[0][j*proxy_w+i].y;
        }
    }
  }
  
  Datastore *proxy_store = set->addStore(proxy_root/"proxy");
  proxy_store->write(proxy_m);
  
  return true;
}
#else
{
  //FIXME report error
  return false;
}
#endif

}
