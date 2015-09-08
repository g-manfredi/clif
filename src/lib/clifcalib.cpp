#include "clifcalib.hpp"

#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;
using namespace boost::filesystem;
using namespace clif_cv;

namespace clif {
    
  bool pattern_detect(ClifDataset *s, int imgset)
  {
    path calib_path("calibration/images/sets");
    CalibPattern pattern;
    
    vector<string> imgsets;
    s->listSubGroups(calib_path, imgsets);
     
    for(int i=0;i<imgsets.size();i++) {
      //int pointcount = 0;
      path cur_path = calib_path / imgsets[i];
      s->getEnum(cur_path / "type", pattern);
      
      vector<vector<Point2f>> ipoints;
      vector<vector<Point2f>> wpoints;
      
      if (pattern == CalibPattern::CHECKERBOARD) {
        Mat img;
        int size[2];
        Datastore *imgs = s->getCalibStore();
        
        s->getAttribute(cur_path / "size", size, 2);
        
        assert(imgs);
        
        //FIXME range!
        for(int j=0;j<imgs->count();j++) {
          vector<Point2f> corners;
          readCvMat(imgs, j, img, CLIF_CVT_8U | CLIF_CVT_GRAY | CLIF_DEMOSAIC);    
          
          int succ = findChessboardCorners(img, Size(size[0],size[1]), corners, CV_CALIB_CB_ADAPTIVE_THRESH+CV_CALIB_CB_NORMALIZE_IMAGE+CALIB_CB_FAST_CHECK+CV_CALIB_CB_FILTER_QUADS);
          
          if (succ)
            printf("found %6d corners (img %d/%d)\n", corners.size(), j, imgs->count());
          else
            printf("found      0 corners (img %d/%d)\n", j, imgs->count());
            
          ipoints.resize(ipoints.size()+1);
          wpoints.resize(wpoints.size()+1);
          
          if (succ) {
            for(int y=0;y<size[1];y++)
              for(int x=0;x<size[0];x++) {
                ipoints.back().push_back(corners[y*size[0]+x]);
                wpoints.back().push_back(Point2f(x,y));
                //pointcount++;
              }
          }
          break;
        }
      }
      else
        abort();
      
      /*//save to hdf5, use single large array and a second info array for subarray, avoids all the dynamic array overhead from hdf5
      float *pointbuf = new float[4*pointcount];
      float *curpoint = pointbuf;
      int *sizebuf = new int[ipoints.size()];
      
      for(int i=0;i<ipoints.size();i++)
        for(int j=0;j<ipoints[i].size();j++) {
          curpoint[0] = ipoints[i][j].x;
          curpoint[1] = ipoints[i][j].y;
          curpoint[2] = wpoints[i][j].x;
          curpoint[3] = wpoints[i][j].y;
          sizebuf[i] = ipoints[i].size();
          curpoint += 4;
        }
        
      s->setAttribute(cur_path / "pointdata", pointbuf, 4*pointcount);
      s->setAttribute(cur_path / "pointcounts", sizebuf, ipoints.size());
      
      delete pointbuf;
      delete sizebuf;*/
      
      writeCalibPoints(s, imgsets[i], ipoints, wpoints);
    }
    
    return false;
  }
  
  bool opencv_calibrate(ClifDataset *f, int flags, int imgset)
  {
    
  }
}