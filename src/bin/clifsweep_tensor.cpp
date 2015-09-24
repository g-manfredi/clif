#include "cliini.h"

#include <cstdlib>
#include <vector>

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE //FIXME need portable extension matcher...
#endif
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "H5Cpp.h"
#include "H5File.h"
#include "opencv2/highgui/highgui.hpp"

#include "clif_cv.hpp"
#include "subset3d.hpp"

using namespace clif;
using namespace H5;
using namespace std;
using namespace cv;

cliini_opt opts[] = {
  {
    "help",
    0, //argcount
    0, //argcount
    CLIINI_NONE, //type
    0, //flags
    'h'
  },
  {
    "input",
    1, //argcount min
    1, //argcount max
    CLIINI_STRING, //type
    0, //flags
    'i'
  },
  {
    "output",
    1, //argcount
    CLIINI_ARGCOUNT_ANY, //argcount
    CLIINI_STRING, //type
    0, //flags
    'o'
  },
  {
    "types", //FIXME remove this
    1, //argcount
    1, //argcount
    CLIINI_STRING, //type
    0, //flags
    't'
  },
  {
    "calib-images",
    1, //argcount
    CLIINI_ARGCOUNT_ANY, //argcount
    CLIINI_STRING
  },
  {
    "detect-patterns",
    0,0
  },
  {
    "calibrate",
    0,0
  }
};

cliini_optgroup group = {
  opts,
  NULL,
  sizeof(opts)/sizeof(*opts),
  0,
  0
};

typedef Vec<ushort, 3> Vec3us;

float scale = 0.25;

int x_step = 1;
int y_step = 1;

void structure_tensor_depth(Mat &epi, Mat &score_m, Mat &depth_m, double d, int l, double f, Subset3d *subset)
{
  int h = epi.size().height;
  vector<Mat> channels;
  
  Mat dx,dy;
  
  split(epi, channels);
  
  for(int c=1;c<2/*channels.size()*/;c++) {
    Mat channel = channels[c];
    
    Scharr(channel, dx, CV_32F, 1, 0);
    Scharr(channel, dy, CV_32F, 0, 1);
    
    //dx *= 1.0/131072;
    //dy *= 1.0/131072;
    Mat dxy;
    
    dxy = dx.mul(dy);
    dx = dx.mul(dx);
    dy = dy.mul(dy);
    
    GaussianBlur(dx, dx, Size(3,h-2), 0);
    GaussianBlur(dy, dy, Size(3,h-2), 0);
    GaussianBlur(dxy, dxy, Size(3,h-2), 0);
    
    /*imwrite("dx.tif", dx*256);
    imwrite("dy.tif", dy*256);
    imwrite("dxy.tif", dxy*256);
    
    exit(0);*/
    if (l == 500) {
      imwrite("epi.tif", channel);
      imwrite("dx.tif", dx/256);
      imwrite("dy.tif", dy/256);
    }
    
    int j = epi.size().height/2;
    for(int i=1;i<epi.size().width-1;i++) {
      double sxx = dx.at<float>(j, i);
      double syy = dy.at<float>(j, i);
      double sxy = dxy.at<float>(j, i);
      double d_y2_x2 = syy-sxx;
      double xy2 = sxy*sxy;
      double a_y2_x2 = syy+sxx;

      double coherence = (d_y2_x2*d_y2_x2+4.0*xy2)/(a_y2_x2*a_y2_x2);
      
      double disp = tan(0.5*atan2(2*sxy, sxx-syy));
      
      if (abs(disp) < 1.0 && coherence > score_m.at<double>(l,i)) {
        depth_m.at<double>(l,i) = subset->disparity2depth(d+disp, scale);
        score_m.at<double>(l,i) = coherence;
      }
    }
  }
}

void write_ply_depth(const char *name, Mat in_d, double f[2], int w, int h, Mat &view, int start, int l)
{
  FILE *pointfile = fopen(name, "w");
  
  Mat d;
  in_d.convertTo(d, CV_32F);
  
  int count = 0;
  
  for(int j=start;j<l;j+=y_step)
    for(int i=0;i<d.size().width;i+=x_step) {
      double depth = d.at<float>(j,i);
      if (depth != 0)
        count++;
    }
  
  fprintf(pointfile, "ply\n"
          "format ascii 1.0\n"
          "element vertex %d\n"
          "property float x\n"
          "property float y\n"
          "property float z\n"
          "property uchar diffuse_red\n"
          "property uchar diffuse_green\n"
          "property uchar diffuse_blue\n"
          "end_header\n", count);
  
  for(int j=start;j<l;j+=y_step)
    for(int i=0;i<d.size().width;i+=x_step) {
      double depth = d.at<float>(j,i);
      if (depth == 0)
        continue;
      Vec3us col = view.at<Vec3us>(j,i);
      fprintf(pointfile, "%.3f %.3f %.3f %d %d %d\n", depth*(i-w/2)/f[0], depth*(j-h/2)/f[1], depth, col[2]/256, col[1]/256, col[0]/256);
    }
  fprintf(pointfile,"\n");
  fclose(pointfile);
}


void write_obj_depth(const char *name, Mat in_d, double f[2], int w, int h, Mat &view, int start, int l)
{
  FILE *pointfile = fopen(name, "w");
  
  Mat d;
  in_d.convertTo(d, CV_32F);
  
  int buf1[w];
  int buf2[w];
  int *valid = buf1;
  int *valid_last = buf2;
  int *valid_tmp;
  
  fprintf(pointfile, "vn 0 0 -1\n");
  
  for(int i=0;i<w;i++)
      valid[i] = 0;
  
  for(int j=0;j<l;j++) {
    valid_tmp = valid_last;
    valid_last = valid;
    valid = valid_tmp;
    for(int i=0;i<w;i++) {
      float depth = d.at<float>(j,i);
      if (depth == 0)
        valid[i] = 0;
      else
        valid[i] = 1;
      Vec3us col = view.at<Vec3us>(j,i);
      fprintf(pointfile, "v %f %f %f %d %d %d\n", depth*(i-w/2)/f[0], depth*(j-h/2)/f[1], depth, col[2]/256, col[1]/256, col[0]/256);
    }
    if (j)
      for(int i=0;i<w-1;i++)
        if (valid[i] && valid[i+1] && valid_last[i] && valid_last[i+1])
          fprintf(pointfile, "f %d %d %d %d\n", (j-1)*w+i,(j-1)*w+i+1,j*w+i+1,j*w+i);

  }
  fprintf(pointfile,"\n");
  fclose(pointfile);
}

int main(const int argc, const char *argv[])
{
  cliini_args *args = cliini_parsopts(argc, argv, &group);

  cliini_arg *input = cliargs_get(args, "input");
  cliini_arg *output = cliargs_get(args, "output");
  
  if (!args || cliargs_get(args, "help\n") || !input || !output) {
    printf("TODO: print help!");
    return EXIT_FAILURE;
  }

  string in_name(cliarg_nth_str(input,0));
  string out_name(cliarg_nth_str(output,0));
  ClifFile f_in(in_name, H5F_ACC_RDONLY);
  Dataset *in_set = f_in.openDataset(0);
  
  Subset3d *slice;
  slice = new Subset3d(in_set);
  
  int size[2];
  
  in_set->imgSize(size);
  
  size[0] *= scale;
  size[1] *= scale;
  
  double focal_length[2];
  
  in_set->get("calibration/intrinsics/checkers/projection", focal_length, 2);
  
  focal_length[0] *= scale;
  focal_length[1] *= scale;
  
  
  Mat img;
  readCvMat(in_set, in_set->Datastore::count()/2, img, UNDISTORT, scale);
  
  Mat epi;
  Mat depth = Mat::zeros(Size(size[0], size[1]), CV_64F);
  Mat score = Mat::zeros(Size(size[0], size[1]), CV_64F);
  for(int l=0/y_step*y_step;l<size[1];l+=y_step) {
    printf("line %d\n", l);
    for(double d=10*scale;d>=1*scale;d-=1) {
      slice->readEPI(epi, l, d, ClifUnit::PIXELS, UNDISTORT, Interpolation::LINEAR, scale);
      structure_tensor_depth(epi, score, depth, d, l, (focal_length[0]+focal_length[1])/2, slice);
    }
  }
  
  Mat d16;
  depth.convertTo(d16, CV_16U);
  imwrite(out_name, d16);
  imwrite("out8bit.tif", depth*0.25);
  
  
  int l = size[1];
  write_ply_depth("points.ply", depth, focal_length, size[0], size[1], img, 0, l);
  write_obj_depth("points.obj", depth, focal_length, size[0], size[1], img, 0, l);
  Mat med, fmat;
  depth.convertTo(fmat, CV_32F);
  medianBlur(fmat, med, 5);
  write_ply_depth("points_m5.ply", med, focal_length, size[0], size[1], img, 0, l);
  write_obj_depth("points_m5.obj", med, focal_length, size[0], size[1], img, 0, l);
  GaussianBlur(med, med, Size(3,3), 0);
  
  write_ply_depth("points_m5_g3.ply", med, focal_length, size[0], size[1], img, 0, l);
  write_obj_depth("points_m5_g3.obj", med, focal_length, size[0], size[1], img, 0, l);

  return EXIT_SUCCESS;
}