#include "cam.hpp"

#include "dataset.hpp"

#ifdef CLIF_WITH_UCALIB
  #include "ucalib/gencam.hpp"
#endif
  
#include <opencv2/imgproc/imgproc.hpp>

namespace clif {

DepthDist::DepthDist(const cpath& path, double at_depth)
  : Tree_Derived_Base<DepthDist>(path), _depth(at_depth)
{
  
}
  
bool DepthDist::load(Dataset *set)
{
  try {
    Idx proxy_size;
    int imgsize[2];
    
    set->getEnum(path()/"type", _type);
        
    if (_type != DistModel::UCALIB)
      return true;
    
    
#ifndef CLIF_WITH_UCALIB
    return true;
#else
      
    //FIXME cams might be missing!
    Mat_<float> corr_line_m = set->readStore(path()/"lines");
    corr_line_m.names({"line","x","y","channels","cams"});
    
    Mat_<float> extrinsics_m;
    
    set->get(path()/"extrinsics", extrinsics_m);
    extrinsics_m.names({"data","channels","cams","views"});
    set->get(path()/"source/source/img_size", imgsize, 2);
    
    _maps.create({IR(imgsize[0], "x"),IR(imgsize[1], "y"), extrinsics_m.r("channels","cams")});
    
    std::cout << "maps:" << _maps << std::endl;
    
    int cam_count = corr_line_m.dim("cams");
    if (cam_count == -1)
      cam_count = 1;
    
    int channels = corr_line_m["channels"];
        
    std::vector<cv::Point3f> ref_points;

    for(auto map_pos : Idx_It_Dims(corr_line_m, "channels","cams")) {
      
      std::cout << "generate map for " << map_pos << std::endl;
      
      std::vector<cv::Vec4d> linefits(corr_line_m["x"]*corr_line_m["y"]);
      
      Idx lines_pos({IR(4, "line"), map_pos.r("x","views")});
        
      for(int j=0;j<corr_line_m["y"];j++)
        for(int i=0;i<corr_line_m["x"];i++)
          for (int e=0;e<4;e++) {
            lines_pos[0] = e;
            lines_pos["x"] = i;
            lines_pos["y"] = j;
            linefits[j*corr_line_m["x"]+i][e] = corr_line_m(lines_pos);
        }
        
      printf("create gencam\n");
      GenCam cam(linefits, cv::Point2i(imgsize[0],imgsize[1]), cv::Point2i(corr_line_m[1],corr_line_m[2]));
      
      
      Idx extr_size(extrinsics_m);
      
      cam.extrinsics.resize(6*extrinsics_m["views"]);
      
      for(auto extr_pos : Idx_It_Dim(extrinsics_m, "views"))
        for(int i=0;i<6;i++) {
          extr_pos["data"] = i;
          extr_pos["channels"] = map_pos["channels"];
          extr_pos["cams"] = map_pos["cams"];
          cam.extrinsics[6*extr_pos["views"]+i] = extrinsics_m(extr_pos);
          std::cout << "extr:" << extr_pos << ": " << extrinsics_m(extr_pos) << std::endl;
        }
        
      
      
      double d = _depth;
      if (isnan(d) || d < 0.1)
        d = 1000000000000.0;
      
      printf("depth: %f\n", d);
      
      cv::Mat map = cvMat(_maps.bind(3, map_pos["cams"]).bind(2, map_pos["channels"]));
      if (ref_points.size())
        cam.get_undist_map_for_depth(map, d, &ref_points);
      else
        cam.get_undist_map_for_depth(map, d, NULL, &ref_points);
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
  
  if (!_maps.size())
    dst = src;
    
  Mat map_pos(_maps);
  
  Idx pos_named = pos;
  
  //FIXME
  if (pos_named.size() == 5)
    pos_named.names({"x","y","channels","cams","views"});
  else if (pos_named.size() == 4)
    pos_named.names({"x","y","channels","views"});
  else
    abort();
  
  Mat maps_cam;
  
  if (pos_named.dim("cams") != -1)
    maps_cam = _maps.bind(3, pos_named["cams"]);
  else
    maps_cam = _maps;
  
  if (_maps.size()) {
    for(int c=0;c<src[2];c++) {
      
      pos_named["channels"] = c;
  
      remap(cvMat(src.bind(2, c)), cvMat(dst.bind(2, c)), cvMat(maps_cam.bind(2, c)), cv::noArray(), interp);
    }
  }
  else
    dst = src;
}

bool DepthDist::operator==(const Tree_Derived & rhs) const
{
  const DepthDist * other = dynamic_cast<const DepthDist*>(&rhs);
  
  if (other && ((other->_depth == _depth) || (isnan(other->_depth) && isnan(_depth))))
    return true;
  
  return false; 
}
  
}