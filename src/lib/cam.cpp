#include "cam.hpp"

#include "dataset.hpp"
#include "ucalib/gencam.hpp"

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
    
    Mat_<float> corr_line_m = set->readStore(path()/"lines");

    set->get(path()/"source/source/img_size", imgsize, 2);
    
    for(int color=0;color<1/*proxy_size[3]*/;color++) {
      std::vector<cv::Vec4d> linefits(corr_line_m[1]*corr_line_m[2]);
      
      printf("load!\n");
      for(int j=0;j<corr_line_m[2];j++)
        for(int i=0;i<corr_line_m[1];i++) {
          linefits[j*corr_line_m[1]+i][0] = corr_line_m(0, i, j, color);
          linefits[j*corr_line_m[1]+i][1] = corr_line_m(1, i, j, color);
          linefits[j*corr_line_m[1]+i][2] = corr_line_m(2, i, j, color);
          linefits[j*corr_line_m[1]+i][3] = corr_line_m(3, i, j, color);
        }
        
      GenCam cam(linefits, cv::Point2i(imgsize[0],imgsize[1]), cv::Point2i(corr_line_m[1],corr_line_m[2]));
      
      
      double d = _depth;
      if (isnan(d))
        //d = std::numeric_limits<double>::max();
        d = 1000;
      printf("calc undist map for depth %f\n", d);
      _map = cam.get_undist_map_for_depth(d);
    }
    
    return true;
  }
  catch (...) {
    return false;
  }
}

void DepthDist::undistort(const clif::Mat & src, clif::Mat & dst)
{
  dst.create(src.type(), src);
  
  for(int c=0;c<src[2];c++)
    remap(cvMat(src.bind(2, c)), cvMat(dst.bind(2, c)), _map, cv::noArray(), cv::INTER_LINEAR);
}

bool DepthDist::operator==(const Tree_Derived & rhs) const
{
  const DepthDist * other = dynamic_cast<const DepthDist*>(&rhs);
  
  if (other && ((other->_depth == _depth) || (isnan(other->_depth) && isnan(_depth))))
    return true;
  
  return false; 
}
  
}