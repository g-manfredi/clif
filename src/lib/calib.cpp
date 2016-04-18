
#include "dataset.hpp"
#include "calib.hpp"
#include "clif_cv.hpp"
#include "enumtypes.hpp"
#include "preproc.hpp"

#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef CLIF_WITH_UCALIB
#include <ucalib/ucalib.hpp>
#include <ucalib/proxy.hpp>
#endif

#ifdef CLIF_WITH_HDMARKER
  #include <hdmarker/hdmarker.hpp>
  #include <hdmarker/subpattern.hpp>
  
  using namespace hdmarker;
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
        proxy_m(pos) *= 10;
      }
    }*/
    
    double rms = fit_cams_lines_multi(proxy_m, cv::Point2i(im_size[0],im_size[1]), lines, extrinsics, extrinsics_rel);
    
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
  }
}
#else
{
  //FIXME report error
  printf("ERROR calibration not available - clif compiled without ucalib!\n");
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
      
      Mat proxy_bound = proxy_m;
      for(int i=proxy_m.size()-1;i>=3;i--)
        proxy_bound = proxy_bound.bind(i, map_pos[i-3]);
      proxy_backwards_poly_generate(proxy_bound, ipoints, wpoints, Point2i(im_size[0], im_size[1]));
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
