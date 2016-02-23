#include "cam.hpp"

#include "dataset.hpp"

#ifdef CLIF_WITH_UCALIB
  #include "ucalib/gencam.hpp"
#endif
  
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

namespace clif {

DepthDist::DepthDist(const cpath& path, double at_depth, int w, int h)
  : Tree_Derived_Base<DepthDist>(path), _depth(at_depth), _w(w), _h(h)
{

}

TD_DistType::TD_DistType(const cpath& path)
  : Tree_Derived_Base<TD_DistType>(path)
{

}

#ifdef CLIF_WITH_UCALIB 
static void _genmap(Mat_<cv::Point2f> & _maps, double _depth, const Idx & map_pos, Mat_<double> &corr_line_m, Mat_<double> &extrinsics_m, int _w, int _h, bool calc_fit, cv::Point2d &_f, cv::Point2d &_m, double &_r)
{
  std::cout << "generate map for " << map_pos << std::endl;
  
  std::vector<cv::Vec4d> linefits(corr_line_m["x"]*corr_line_m["y"]);
  
  Idx lines_pos({IR(4, "line"), map_pos.r("x","cams")});
  
  for(int j=0;j<corr_line_m["y"];j++)
    for(int i=0;i<corr_line_m["x"];i++)
      for (int e=0;e<4;e++) {
        lines_pos[0] = e;
        lines_pos["x"] = i;
        lines_pos["y"] = j;
        linefits[j*corr_line_m["x"]+i][e] = corr_line_m(lines_pos);
      }
      
  printf("create gencam\n");
  
  GenCam cam;
  
  double d = _depth;
  if (isnan(d) || d < 0.1)
    d = 1000000000000.0;
  
  printf("depth: %f\n", d);
  
  if (calc_fit) {
    cam = GenCam(linefits, cv::Point2i(_w,_h), cv::Point2i(corr_line_m[1],corr_line_m[2]), d);
    _f = cam.f;
    _m = cam.move;
    _r = cam.rot;
  }
  else
    cam = GenCam(linefits, cv::Point2i(_w,_h), cv::Point2i(corr_line_m[1],corr_line_m[2]), d, _f, _m, _r);
    
    
  printf("%fx%f %fx%f %f\n", _f.x, _f.y, _m.x, _m.y, _r);
  
  cv::Mat map = cvMat(_maps.bind(3, map_pos["cams"]).bind(2, map_pos["channels"]));
  cam.get_undist_map_for_depth(map, d);
}
#endif
  

bool TD_DistType::load(Dataset *set)
{
  try {
    set->getEnum(path()/"type", _type);
        
    return true;
  }
  catch (std::invalid_argument) {
    return false;
  }
}
  
bool DepthDist::load(Dataset *set)
{
  try {
    Idx proxy_size;
    
    set->getEnum(_path/"type", _type);
    
    if (_type == DistModel::CV8) {
      double f[2], c[2];
      
      f[0] = 0;
      f[1] = 0;
      c[0] = 0;
      c[1] = 0;
      cv::Mat cv_cam = cv::Mat::eye(3,3,CV_64F);
      std::vector<double> cv_dist;
  
      set->get(_path / "projection", f, 2);
      cv_cam.at<double>(0,0) = f[0];
      cv_cam.at<double>(1,1) = f[1];
    
      Attribute *a = set->get(_path / "projection_center");
      if (a) {
        a->get(c, 2);
        //FIXME causes very misaligned undistortion?
        //cv_cam.at<double>(0,2) = c[0];
        //cv_cam.at<double>(1,2) = c[1];
        cv_cam.at<double>(0,2) = _w/2;
        cv_cam.at<double>(1,2) = _h/2;
      }
    
      set->get(_path / "opencv_distortion", cv_dist);

      _map.create({IR(_w, "x"),IR(_h, "y")});
  
      cv::Mat tmp;
      cv::initUndistortRectifyMap(cv_cam, cv_dist, cv::noArray(), cv::noArray(), cv::Size(_w,_h), CV_32FC2, cvMat(_map), tmp);
  
      return true;
    }
    
    if (_type != DistModel::UCALIB)
      return true;
    
    
#ifndef CLIF_WITH_UCALIB
    return true;
#else
          
    //FIXME cams might be missing!
    Mat_<double> corr_line_m = set->readStore(path()/"lines");
    corr_line_m.names({"line","x","y","channels","cams"});
    
    Mat_<double> extrinsics;
    
    set->get(path()/"extrinsics_cams", extrinsics);
    extrinsics.names({"extrinsics","channels","cams"});
    
    _maps.create({IR(_w, "x"),IR(_h, "y"), extrinsics.r("channels","cams")});
    
    double extrinsics_main[6];
    for(int i=0;i<6;i++)
      extrinsics_main[i] = extrinsics(Idx({i,extrinsics["channels"]/2,extrinsics["cams"]/2}));
    
    
    _genmap(_maps, _depth, Idx({IR(0,"line"),IR(0,"x"),IR(0,"y"),IR(corr_line_m["channels"]/2,"channels"),IR(corr_line_m["cams"]/2,"cams")}), corr_line_m, extrinsics, _w, _h, true, _f, _m, _r);
    
    //center rotation and translation
    cv::Matx31f c_r(extrinsics_main[0],extrinsics_main[1],extrinsics_main[2]);
    cv::Matx31f c_t(extrinsics_main[3],extrinsics_main[4],extrinsics_main[5]);
    cv::Matx33f c_r_m;
    cv::Rodrigues(c_r, c_r_m);
    
    std::cout << "extr center rotate: \n" << c_r << "\nextr center translate:\n" << c_t << std::endl;
    
    cv::Matx31f ref(0, 0, -1000);
    cv::Matx31f ref_c = ref + c_t;
    cv::Matx21f res_c(ref_c(0)*_f.x/ref_c(2),ref_c(1)*_f.y/ref_c(2));
    
    cv::Matx21f avg_dir(0, 0);
    

    //FIXME skip center view!
    for(auto pos : Idx_It_Dims(extrinsics, "channels","cams")) {
      //to move lines from local to ref view do:
      //undo local translation
      //undo local rotation (we are now in target coordinates)
      //do ref rotation
      //do ref translation
      cv::Matx31f l_r(extrinsics({0,pos.r("channels","cams")}),
                      extrinsics({1,pos.r("channels","cams")}),
                      extrinsics({2,pos.r("channels","cams")}));
      cv::Matx31f l_t(extrinsics({3,pos.r("channels","cams")}),
                      extrinsics({4,pos.r("channels","cams")}),
                      extrinsics({5,pos.r("channels","cams")}));
      cv::Matx33f l_r_m, l_r_m_t;
      cv::Rodrigues(l_r, l_r_m);
      transpose(l_r_m, l_r_m_t);
      
      std::cout << "cam rotate: \n" << l_r << "\ncam translate:\n" << l_t << std::endl;
      
      
      /*if (pos["channels"] == extrinsics_m["channels"]/2
        && pos["cams"] == extrinsics_m["cams"]/2)
        printf("shoud stay the same!                                       !!!!!!!!!!!!!!!!!!!!!\n");*/
      
      std::cout << pos << std::endl;
      
      
      cv::Matx33f rotm = c_r_m*l_r_m_t;
      
      
      //calculate after-projection rotation angle - to align array dir with view horizon
      //move a point parallel to each cam/view project it, calc avg angle towards center view projected point
      cv::Matx31f ref_l = ref + l_t;
      cv::Matx21f res(ref_l(0)*_f.x/ref_l(2),ref_l(1)*_f.y/ref_l(2));
      
      std::cout << "ref_l: " << ref_l << std::endl;
      std::cout << "ref: " << res - res_c << std::endl;
      
      normalize(res-res_c, res);
      
      if (pos["cams"] < extrinsics["cams"]/2)
        avg_dir -= res;
      else if (pos["cams"] > extrinsics["cams"]/2)
        avg_dir += res;

      
      for(auto pos_lines : Idx_It_Dims(corr_line_m, "x", "y")) {
        cv::Matx31f offset(corr_line_m({0, pos_lines.r("x","y"), pos.r("channels","cams")}),
                           corr_line_m({1, pos_lines.r("x","y"), pos.r("channels","cams")}),
                           0.0);
        cv::Matx31f dir(corr_line_m({2, pos_lines.r("x","y"), pos.r("channels","cams")}),
                           corr_line_m({3, pos_lines.r("x","y"), pos.r("channels","cams")}),
                           1.0);
        
        
        /*if (pos_lines["x"] == 32 && pos_lines["y"] == 24) {
          std::cout << "start: " << offset << "\n" << dir << std::endl;
          
          
          std::cout << "rot1: " << l_r_m_t << std::endl;
          std::cout << "rot2: " << c_r_m << std::endl;
          std::cout << "rot1*rot2: " << c_r_m*l_r_m_t << std::endl;
        }*/
        
        offset -= l_t;
        offset = rotm * offset;
        //offset = l_r_m_t * offset;
        //offset = c_r_m * offset;
        offset += c_t;
        
        //dir -= l_t;
        dir = rotm*dir;
        //dir = l_r_m_t * offset;
        //dir = c_r_m * offset;
        //dir += c_t;
        
        //normalize
        dir *= 1.0/dir(2);
        offset -= offset(2)*dir;
        
        /*if (pos_lines["x"] == 32 && pos_lines["y"] == 24)
          std::cout << "res: " << offset << dir << std::endl;*/
        
        corr_line_m({0, pos_lines.r("x","y"), pos.r("channels","cams")}) = offset(0);
        corr_line_m({1, pos_lines.r("x","y"), pos.r("channels","cams")}) = offset(1);
        
        corr_line_m({2, pos_lines.r("x","y"), pos.r("channels","cams")}) = dir(0);
        corr_line_m({3, pos_lines.r("x","y"), pos.r("channels","cams")}) = dir(1);
      }
      
    }
    
    std::cout << "avg dir:" << avg_dir << "\n" << atan2(avg_dir(1),avg_dir(0))/M_PI*180 << "°" << std::endl;
    
    _r = atan2(avg_dir(1),avg_dir(0));
    
    std::cout << "maps:" << _maps << std::endl;
    
    int cam_count = corr_line_m.dim("cams");
    if (cam_count == -1)
      cam_count = 1;
    
    int channels = corr_line_m["channels"];
        
    std::vector<cv::Point3f> ref_points;
    
    for(auto map_pos : Idx_It_Dims(corr_line_m, "channels","cams")) {
      _genmap(_maps, _depth, map_pos, corr_line_m, extrinsics, _w, _h, false, _f, _m, _r);
    }
    
    return true;
#endif
  }
  catch (std::invalid_argument) {
    return false;
  }
}

void DepthDist::undistort(const clif::Mat & src, clif::Mat & dst, const Idx & pos, int interp)
{
  dst.create(src.type(), src);
  
  if (interp == -1)
    interp = cv::INTER_LINEAR;
  
  if (_type == DistModel::CV8) {
    for(int c=0;c<src[2];c++)
      remap(cvMat(src.bind(2, c)), cvMat(dst.bind(2, c)), cvMat(_map), cv::noArray(), interp);
  }
  else if (_type == DistModel::UCALIB) {
    if (!_maps.size()) {
      dst = src;
      return;
    }
      
    Mat map_pos(_maps);
    
    Idx pos_named = pos;
    
    //FIXME
    if (pos_named.size() == 5)
      pos_named.names({"x","y","channels","cams","views"});
    else if (pos_named.size() == 4)
      pos_named.names({"x","y","channels","cams"});
    else
      abort();
    
    Mat maps_cam;
    
    if (pos_named.dim("cams") != -1)
      maps_cam = _maps.bind(3, pos_named["cams"]);
    else
      maps_cam = _maps;
  
    for(int c=0;c<src[2];c++) {
      
      
      
      cv::Mat src_dup = cvMat(src.bind(2, c)).clone();
      /*
      for(int i=0;i<4;i++) {
        cv::Point2f ref = _ref_point_proj(i, pos_named["channels"], pos_named["cams"]);
        circle(src_dup, ref*32, 30, 128+127*i/3, 10, CV_AA, 5);
      }*/
      
      pos_named["channels"] = c;
      remap(src_dup, cvMat(dst.bind(2, c)), cvMat(maps_cam.bind(2, c)), cv::noArray(), interp);
      
      
      
      //pos_named["channels"] = c;
      //remap(cvMat(src.bind(2, c)), cvMat(dst.bind(2, c)), cvMat(maps_cam.bind(2, c)), cv::noArray(), interp);
    }
  }
  else {
    dst = src;
  }
}

bool DepthDist::operator==(const Tree_Derived & rhs) const
{
  const DepthDist * other = dynamic_cast<const DepthDist*>(&rhs);
  
  if (other && ((other->_depth == _depth) || (isnan(other->_depth) && isnan(_depth))))
    return true;
  
  return false; 
}

bool TD_DistType::operator==(const Tree_Derived & rhs) const
{
  const TD_DistType * other = dynamic_cast<const TD_DistType*>(&rhs);
  
  if (other)
    return true;
  
  return false; 
}
  
}