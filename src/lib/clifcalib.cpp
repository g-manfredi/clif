#include "clifcalib.hpp"

#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;
using namespace boost::filesystem;
using namespace clif_cv;

namespace clif {
    
  bool pattern_detect(Dataset *s, int imgset)
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
          
          if (succ) {
            printf("found %6d corners (img %d/%d)\n", corners.size(), j, imgs->count());
            cornerSubPix(img, corners, Size(8,8), Size(-1,-1), TermCriteria(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS,100,0.0001));
          }
          else
            printf("found      0 corners (img %d/%d)\n", j, imgs->count());
            
          ipoints.push_back(std::vector<Point2f>());
          wpoints.push_back(std::vector<Point2f>());
          
          if (succ) {
            for(int y=0;y<size[1];y++)
              for(int x=0;x<size[0];x++) {
                ipoints.back().push_back(corners[y*size[0]+x]);
                wpoints.back().push_back(Point2f(x,y));
                //pointcount++;
              }
          }
        }
      }
      else
        abort();
      
      writeCalibPoints(s, imgsets[i], ipoints, wpoints);
    }
    
    return false;
  }
  
  bool opencv_calibrate(Dataset *set, int flags, std::string imgset, std::string calibset)
  {
    Mat cam;
    vector<double> dist;
    vector<Mat> rvecs;
    vector<Mat> tvecs;
    
    vector<vector<Point2f>> ipoints_read;
    vector<vector<Point2f>> wpoints_read;
    vector<vector<Point2f>> ipoints;
    vector<vector<Point3f>> wpoints;
    
    if (!imgset.size()) {
      vector<string> imgsets;
      set->listSubGroups("calibration/images/sets", imgsets);
      assert(imgsets.size());
      imgset = imgsets[0];
    }
    
    if (!calibset.size())
      calibset = imgset;
      
    readCalibPoints(set, imgset, ipoints_read, wpoints_read);
    for(int i=0;i<wpoints_read.size();i++) {
      if (!wpoints_read[i].size())
        continue;
      ipoints.push_back(std::vector<Point2f>(wpoints_read[i].size()));
      wpoints.push_back(std::vector<Point3f>(wpoints_read[i].size()));
      for(int j=0;j<wpoints_read[i].size();j++) {
        wpoints.back()[j] = Point3f(wpoints_read[i][j].x,wpoints_read[i][j].y,0);
        ipoints.back()[j] = ipoints_read[i][j];
      }
    }
    
    double rms = calibrateCamera(wpoints, ipoints, imgSize(set), cam, dist, rvecs, tvecs, flags);
    
    
    printf("opencv calibration rms %f\n", rms);
    
    std::cout << cam << std::endl;
    
    double f[2] = { cam.at<double>(0,0), cam.at<double>(1,1) };
    double c[2] = { cam.at<double>(0,2), cam.at<double>(1,2) };
    
    //FIXME todo delete previous group!
    boost::filesystem::path calib_path;
    calib_path /= "calibration/intrinsics" / calibset;
    set->setAttribute(calib_path / "type", "CV8");
    set->setAttribute(calib_path / "projection", f, 2);
    set->setAttribute(calib_path / "projection_center", c, 2);
    set->setAttribute(calib_path / "opencv_distortion", dist);
  }
}