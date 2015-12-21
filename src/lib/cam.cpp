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
    
    /*Mat_<float> corr_line_m = set->readStore(path()/"lines");
    Mat_<float> extrinsics_m;
    
    set->get(path()/"extrinsics", extrinsics_m);

    set->get(path()/"source/source/img_size", imgsize, 2);
    
    //_maps.resize(corr_line_m[3]);
    
    int cam_count = 1;
    int channels = corr_line_m[3];
    
    if (corr_line_m.size() == 5)
      cam_count = corr_line_m[4];
    
    if (cam_count == 1)
      _maps.create(Idx{imgsize[0], imgsize[1], channels, cam_count});
        
    std::vector<cv::Point3f> ref_points;
    
    Idx map_pos(4);
    
    for(;map_pos<corr_line_m;map_pos.step(2,corr_line_m)) {
      std::vector<cv::Vec4d> linefits(corr_line_m[1]*corr_line_m[2]);
      
      Idx lines_pos(_maps.size()+1);
      for(int i=1;i<lines_pos.size();i++)
        lines_pos[i] = map_pos[i-1];
        
      for(int j=0;j<corr_line_m[2];j++)
        for(int i=0;i<corr_line_m[1];i++)
          for (int e=0;e<4;e++) {
          lines_pos[0] = e;
          linefits[j*corr_line_m[1]+i][e] = corr_line_m(lines_pos);
        }
        
      GenCam cam(linefits, cv::Point2i(imgsize[0],imgsize[1]), cv::Point2i(corr_line_m[1],corr_line_m[2]));
      
      
      Idx extr_size(extrinsics_m.size());
      extr_size[0] = 6;
      for(int i=1;i<extr_size.size();i++)
        extr_size[i] = extrinsics_m[i];
      
      int extr_views_dim = extrinsics_m.size()-1;
      
      cam.extrinsics.resize(6*extrinsics_m[extr_views_dim]);
      
      Idx extr_pos(extr_size.size());
      for(int i=1;i<extr_pos.size();i++)
        extr_pos[i] = map_pos[i+2];
      for(int img_n=0;img_n<extrinsics_m[extr_views_dim];img_n++)
        for(int i=0;i<6;i++) {
          cam.extrinsics[6*img_n+i] = extrinsics_m(i, color, cam, img_n);
        }
        
      
      
      double d = _depth;
      if (isnan(d) || d < 0.1)
        d = 1000000000000.0;
      
      cam.get_undist_map_for_depth(cvMat(_maps.bind(3, map_pos[3]).bind(2, map_pos[2])), d, &ref_points);
    }*/
    
    return true;
#endif
  }
  catch (std::invalid_argument) {
    return false;
  }
}

void DepthDist::undistort(const clif::Mat & src, clif::Mat & dst, int interp)
{
  dst.create(src.type(), src);
  
  if (interp == -1)
    interp = cv::INTER_LINEAR;
  
  if (!_maps.size())
    dst = src;
    
  if (_maps.size())
    for(int c=0;c<src[2];c++)
      remap(cvMat(src.bind(2, c)), cvMat(dst.bind(2, c)), _maps[c], cv::noArray(), interp);
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