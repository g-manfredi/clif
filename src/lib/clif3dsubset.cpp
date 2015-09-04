#include "clif3dsubset.hpp"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace clif;
using namespace clif_cv;
using namespace cv;

Clif3DSubset::Clif3DSubset(ClifDataset *data, std::string extr_group)
: _data(data)
{
  string root("calibration/extrinsics/");
  root = root.append(extr_group);
  
  ExtrType type;
  
  data->getEnum((root+"/type").c_str(), type);
  
  assert(type == ExtrType::LINE);
  
  double line_step[3];
  
  data->getAttribute(root+"/line_step", line_step, 3);
  
  //TODO for now we only support horizontal lines!
  assert(line_step[0] != 0.0);
  assert(line_step[1] == 0.0);
  assert(line_step[2] == 0.0);

  step_length = line_step[0];
  
  //TODO which intrinsic to select!
  
  vector<string> intrs = data->listSubGroups("calibration/intrinsics");
  
  data->getAttribute("calibration/intrinsics/"+intrs[0]+"/projection", f, 2);
}


void Clif3DSubset::readEPI(cv::Mat &m, int line, double depth, int flags)
{      
  double step = f[0]*step_length/depth;
  
  cv::Mat tmp;
  readCvMat(_data, 0, tmp, flags | CLIF_DEMOSAIC);

  m = cv::Mat::zeros(cv::Size(imgSize(_data).width, _data->imgCount()), tmp.type());
  
  for(int i=0;i<_data->imgCount();i++)
  {      
    //FIXME rounding?
    double d = step*(i-_data->imgCount()/2);
    
    if (abs(d) >= tmp.size().width)
      continue;
    
    readCvMat(_data, i, tmp, flags | CLIF_DEMOSAIC);
    
    Matx23d warp(1,0,d, 0,1,0);
    warpAffine(tmp.row(line), m.row(i), warp, m.row(i).size(), CV_INTER_LANCZOS4);
  }
}