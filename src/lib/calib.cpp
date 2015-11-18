#include "calib.hpp"
#include "clif_cv.hpp"
#include "enumtypes.hpp"
#include "dataset.hpp"

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
#endif
  
using namespace std;
using namespace cv;

namespace clif {
  
typedef unsigned int uint;

  bool pattern_detect(Dataset *s, int imgset, bool write_debug_imgs)
  {
    path calib_path("calibration/images/sets");
    CalibPattern pattern;
    
    vector<string> imgsets;
    s->listSubGroups(calib_path, imgsets);
    
     
    for(uint i=0;i<imgsets.size();i++) {
      Datastore *debug_store = NULL;
      //int pointcount = 0;
      path cur_path = calib_path / imgsets[i];
      s->getEnum(cur_path / "type", pattern);
      
      if (write_debug_imgs && pattern == CalibPattern::HDMARKER) {
        debug_store = s->addStore(cur_path / "debug_images");
        debug_store->setDims(4);
      }
      
      vector<vector<Point2f>> ipoints;
      vector<vector<Point2f>> wpoints;
      
      //images(1D+), channels - at least 2d
      /*int idx[2];
      Mat_<std::vector<float>> wpoints_m;
      Mat_<std::vector<float>> ipoints_m;*/
      
      if (pattern == CalibPattern::CHECKERBOARD) {
        Mat img;
        int size[2];
        Datastore *imgs = s->getCalibStore();
        
        s->get(cur_path / "size", size, 2);
        
        assert(imgs);
        assert(imgs->dims() == 4);
        
        /*printf("imgcount: %d %d %d\n", imgs->imgCount(), imgs->imgChannels(), sizeof(std::vector<float>));
        
        
        wpoints_m = Mat(imgs->imgCount(), imgs->imgChannels(), DataType<std::vector<float>>::type);
        ipoints_m = Mat_<std::vector<float>>(imgs->imgCount(), imgs->imgChannels());
        
        printf("element size?! : %d\n", wpoints_m.elemSize());
        
        new(wpoints_m.data) std::vector<float>[wpoints_m.total()]();
        new(ipoints_m.data) std::vector<float>[ipoints_m.total()]();*/
        //FIXME delete
        
        //FIXME range!
        for(int j=0;j<imgs->imgCount();j++) {
          vector<Point2f> corners;
          std::vector<int> idx(4, 0);
          idx[3] = j;
          imgs->readImage(idx, &img, CVT_8U | CVT_GRAY | DEMOSAIC);
          
          cv::Mat ch = clifMat_channel(img, 0);
          
          int succ = findChessboardCorners(ch, Size(size[0],size[1]), corners, CV_CALIB_CB_ADAPTIVE_THRESH+CV_CALIB_CB_NORMALIZE_IMAGE+CALIB_CB_FAST_CHECK+CV_CALIB_CB_FILTER_QUADS);
          
          if (succ) {
            printf("found %6lu corners (img %d/%d)\n", corners.size(), j, imgs->imgCount());
            cornerSubPix(ch, corners, Size(8,8), Size(-1,-1), TermCriteria(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS,100,0.0001));
          }
          else
            printf("found      0 corners (img %d/%d)\n", j, imgs->imgCount());
            
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
#ifdef CLIF_WITH_HDMARKER
      else if (pattern == CalibPattern::HDMARKER) {
        Mat img;
        Datastore *imgs = s->getCalibStore();
        
        double unit_size; //marker size in mm
        double unit_size_res;
        int recursion_depth;
        
        s->get(cur_path / "marker_size", unit_size);
        s->get(cur_path / "hdmarker_recursion", recursion_depth);
        
        //FIXME remove this!
        Marker::init();
        
        
        assert(imgs);
        
        //FIXME range!
        
        assert(imgs->dims() == 4);
        
        for(int j=0;j<imgs->imgCount();j++) {
          std::vector<Corner> corners_rough;
          std::vector<Corner> corners;
          std::vector<int> idx(4, 0);
          idx[3] = j;
          bool *mask_ptr = NULL;
          bool masks[3][4];
          
          Mat *debug_img_ptr = NULL;
          Mat debug_img;          
          
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
              default :
                abort();
            }
            
            
            Mat debug_imgs[3];
            
            //grayscale rough detection
            imgs->readImage(idx, &img, CVT_8U | CVT_GRAY | DEMOSAIC);
            cv::Mat gray = clifMat_channel(img, 0);
            Marker::detect(gray, corners_rough);
            
            imgs->readImage(idx, &img, CVT_8U);
            cv::Mat bayer = clifMat_channel(img, 0);
            
            char buf[128];
            
            sprintf(buf, "orig_img%03d.tif", j);
            imwrite(buf, bayer);
            
            for(int c=0;c<3;c++) {
              if (debug_store)
                debug_img_ptr = &debug_imgs[c];
              
              unit_size_res = unit_size;
              mask_ptr = &masks[c][0];
              hdmarker_detect_subpattern(bayer, corners_rough, corners, recursion_depth, &unit_size_res, debug_img_ptr, mask_ptr, 0);
              
              printf("found %6lu corners for channel %d (img %d/%d)\n", corners.size(), c, j, imgs->imgCount());
              
              sprintf(buf, "debug_img%03d_ch%d.tif", j, c);
              imwrite(buf, *debug_img_ptr);
            }
            
            if (debug_store)
              cv::merge(debug_imgs, 3, debug_img);
          }
          else {
            
            if (debug_store)
              debug_img_ptr = &debug_img;
            
            imgs->readImage(idx, &img, CVT_8U | CVT_GRAY | DEMOSAIC);
            
            cv::Mat ch = clifMat_channel(img, 0);
            
            Marker::detect(ch, corners);
            //FIXME use input size
            //FIXME use input depht
            unit_size_res = unit_size;
            hdmarker_detect_subpattern(ch, corners, corners, recursion_depth, &unit_size_res, debug_img_ptr, 0);
            
            printf("found %6lu corners (img %d/%d)\n", corners.size(), j, imgs->imgCount());
          }
          
          if (debug_store) {
            debug_store->appendImage(&debug_img);
            
            char buf[128];
            sprintf(buf, "col_fit_img%03d.tif", j);
            imwrite(buf, debug_img);
          }

          ipoints.push_back(std::vector<Point2f>());
          wpoints.push_back(std::vector<Point2f>());
          
          for(int c=0;c<corners.size();c++) {
            //FIXME multi-channel calib!
            ipoints.back().push_back(corners[c].p);
            wpoints.back().push_back(unit_size_res*Point2f(corners[c].id.x, corners[c].id.y));
            //pointcount++;
          }
        }
      }
#endif
      else
        abort();
      
      //FIXME put into datastore!
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
    for(uint i=0;i<wpoints_read.size();i++) {
      if (!wpoints_read[i].size())
        continue;
      ipoints.push_back(std::vector<Point2f>(wpoints_read[i].size()));
      wpoints.push_back(std::vector<Point3f>(wpoints_read[i].size()));
      for(uint j=0;j<wpoints_read[i].size();j++) {
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
    path calib_path;
    calib_path /= "calibration/intrinsics";
    calib_path /= calibset;
    set->setAttribute(calib_path / "type", "CV8");
    set->setAttribute(calib_path / "projection", f, 2);
    set->setAttribute(calib_path / "projection_center", c, 2);
    set->setAttribute(calib_path / "opencv_distortion", dist);
    
    return true;
  }
  


  bool generate_proxy_loess(Dataset *set, int proxy_w, int proxy_h , std::string imgset, std::string calibset)
#ifdef CLIF_WITH_UCALIB
  {
    Cam_Config cam_config = { 0.0065, 12.0, 300.0, -1, -1 };
    Calib_Config conf = { true, 190, 420 };

    cam_config.w = set->getCalibStore()->extent()[0];
    cam_config.h = set->getCalibStore()->extent()[1];
    
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
    for(uint i=0;i<wpoints_read.size();i++) {
      if (!wpoints_read[i].size())
        continue;
      ipoints.push_back(std::vector<Point2f>(wpoints_read[i].size()));
      wpoints.push_back(std::vector<Point3f>(wpoints_read[i].size()));
      for(uint j=0;j<wpoints_read[i].size();j++) {
        wpoints.back()[j] = Point3f(wpoints_read[i][j].x,wpoints_read[i][j].y,0);
        ipoints.back()[j] = ipoints_read[i][j];
      }
      
      printf("%d points\n", wpoints_read[i].size());
      printf("%fx%f points\n", wpoints[i][0].x,wpoints[i][0].y);
    }
    
    Point2i proxy_size(proxy_w,proxy_h);
    
    DistCorrLines dist_lines = DistCorrLines(0, 0, 0, cam_config.w, cam_config.h, 100.0, cam_config, conf, proxy_size);
    dist_lines.add(ipoints, wpoints, 0.0);
    dist_lines.proxy_backwards_poly_generate();
    
    cv::Mat proxy_img;
    proxy_img.create(Size(proxy_size.x, proxy_size.y), CV_32FC2);
    Datastore *proxy_store = set->addStore(path("calibration/images/sets") / calibset / "proxy");
    proxy_store->setDims(4);
    
    for(int img_n=0;img_n<ipoints.size();img_n++) {
      printf("%fx%f\n", dist_lines.proxy_backwards[img_n][0].x, dist_lines.proxy_backwards[img_n][0].y);
      for(int j=0;j<proxy_size.y;j++)
        for(int i=0;i<proxy_size.x;i++) {
          proxy_img.at<Point2f>(j,i) = dist_lines.proxy_backwards[img_n][j*proxy_size.x+i];
        }
      proxy_store->appendImage(&proxy_img);
    }
        
        
    
    /*printf("opencv calibration rms %f\n", rms);
    
    std::cout << cam << std::endl;
    
    double f[2] = { cam.at<double>(0,0), cam.at<double>(1,1) };
    double c[2] = { cam.at<double>(0,2), cam.at<double>(1,2) };
    
    //FIXME todo delete previous group!
    path calib_path;
    calib_path /= "calibration/intrinsics";
    calib_path /= calibset;
    set->setAttribute(calib_path / "type", "CV8");
    set->setAttribute(calib_path / "projection", f, 2);
    set->setAttribute(calib_path / "projection_center", c, 2);
    set->setAttribute(calib_path / "opencv_distortion", dist);*/
    
    return true;
  }
#else
  {
    //FIXME report error
    return false;
  }
#endif

}