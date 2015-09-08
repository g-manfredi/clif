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

#include "clif.hpp"
#include "clif3dsubset.hpp"

using namespace clif_cv;
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

int x_step = 1;
int y_step = 16;

int minx = 350/x_step*x_step;
int maxx = 1150;

int grad_f_thres = 50000;

static inline int diffsum_2(Vec3us a, Vec3us b)
{
  Vec3i d(a[0]-b[0],a[1]-b[1],a[2]-b[2]);
  
  return d[0]*d[0]+d[1]*d[1]+d[2]*d[2];
}

static inline int diffsum(Vec3i a, Vec3i b)
{
  return abs(a[0]-b[0]) + abs(a[1]-b[1]) + abs(a[2]-b[2]);
}

static inline double score_range(Mat &epi, int x, int y_start, int y_end)
{
  double score = 1000000000000;
  
  Vec3i avg(0,0,0);
  for(int j=y_start;j<=y_end;j++)
    avg += epi.at<Vec3us>(j,x);
  
  avg *= 1.0/(y_end-y_start+1);

  for(int j=y_start;j<=y_end;j++)
    score -= norm(avg-(Vec3i)epi.at<Vec3us>(j,x), NORM_L1);
  
  for(int j=y_start;j<=y_end-1;j++)
    score -= diffsum(epi.at<Vec3us>(j,x), epi.at<Vec3us>(j+1,x));

  return score;
}

void score_epi(Mat &epi, Mat &score_m, Mat &depth_m, double d, int l)
{
  int h = epi.size().height;
  
#pragma omp parallel for schedule(dynamic)
  for(int i=std::max(minx/x_step*x_step,x_step);i<std::min(epi.size().width-1, maxx);i+=x_step) {
    Vec3i vx(0,0,0);
    double gy = 0;
    double score = 1000000000000;
    int count = 0;
    
    /*  Vec3i ref = (Vec3i)epi.at<Vec3us>(h/2,i) - (Vec3i)epi.at<Vec3us>(h/2,i+1);
    
    for(int j=0;j<=h-1;j++) {
      if (diffsum_i(ref, (Vec3i)epi.at<Vec3us>(j,i)-(Vec3i)epi.at<Vec3us>(j,i+1)) < grad_f_thres) {
        gx = diffsum(epi.at<Vec3us>(j,i), epi.at<Vec3us>(j,i+1));
        gy = diffsum(epi.at<Vec3us>(j,i), epi.at<Vec3us>(j+1,i));
        score += gx/(gy+1);
        count++;
      }
    }
    
    score /= count;*/
    
    /*for(int j=0;j<=h-1;j++) {
      vx += (Vec3i)epi.at<Vec3us>(j,i)-(Vec3i)epi.at<Vec3us>(j,i+1);
      gy += diffsum(epi.at<Vec3us>(j,i), epi.at<Vec3us>(j+1,i));
    }
     
    score = (abs(vx[0])+abs(vx[1])+abs(vx[2]))/(gy+h*1000);*/
    
    /*Vec3i avg(0,0,0);
    for(int j=0;j<h;j++)
      avg += epi.at<Vec3us>(j,i);
    
    avg *= 1.0/h;

    for(int j=0;j<h;j++)
      score -= norm(avg-(Vec3i)epi.at<Vec3us>(j,i), NORM_L1);
    
    for(int j=0;j<h-1;j++)
      score -= diffsum(epi.at<Vec3us>(j,i), epi.at<Vec3us>(j+1,i));*/
    
    score = score_range(epi, i, 0, h/2);
    
    if (score > score_m.at<double>(l,i)) {
      score_m.at<double>(l,i) = score;
      depth_m.at<double>(l,i) = d;
    }
    
    score = score_range(epi, i, h/2, h-1);
    
    if (score > score_m.at<double>(l,i)) {
      score_m.at<double>(l,i) = score;
      depth_m.at<double>(l,i) = d;
    }
    
    /*for(int j=h/4;j<=h/2;j++) {
      gx = abs(norm((Vec3i)epi.at<Vec3us>(j,i-1) - (Vec3i)epi.at<Vec3us>(j,i+1), NORM_L1));
      gy = abs(norm((Vec3i)epi.at<Vec3us>(j-1,i) - (Vec3i)epi.at<Vec3us>(j+1,i), NORM_L1));
      score += gx/(gy+1);
    }
     
     
    //score = gx/(gy+0.01);
//#pragma omp critical
    if (score > score_m.at<double>(l,i)) {
      score_m.at<double>(l,i) = score;
      depth_m.at<double>(l,i) = d;
    }
    
    gx = 0;
    gy = 0;
    score = 0;
    for(int j=h*3/8;j<=h*5/8;j++) {
      gx = abs(norm((Vec3i)epi.at<Vec3us>(j,i-1) - (Vec3i)epi.at<Vec3us>(j,i+1), NORM_L1));
      gy = abs(norm((Vec3i)epi.at<Vec3us>(j-1,i) - (Vec3i)epi.at<Vec3us>(j+1,i), NORM_L1));
      score += gx/(gy+1);
    }
     
    score = gx/(gy+0.01);
//#pragma omp critical
    if (score > score_m.at<double>(l,i)) {
      score_m.at<double>(l,i) = score;
      depth_m.at<double>(l,i) = d;
    }
    
    gx = 0;
    gy = 0;
    score = 0;
    for(int j=h/2;j<=h-h/4;j++) {
      gx = abs(norm((Vec3i)epi.at<Vec3us>(j,i-1) - (Vec3i)epi.at<Vec3us>(j,i+1), NORM_L1));
      gy = abs(norm((Vec3i)epi.at<Vec3us>(j-1,i) - (Vec3i)epi.at<Vec3us>(j+1,i), NORM_L1));
      score += gx/(gy+1);
    }
     
     
    score = gx/(gy+0.01);
//#pragma omp critical
    if (score > score_m.at<double>(l,i)) {
      score_m.at<double>(l,i) = score;
      depth_m.at<double>(l,i) = d;
    }*/
    
  }
}

void write_obj_depth(const char *name, Mat in_d, double f[2], int w, int h, Mat &view, int start, int l)
{
  FILE *pointfile = fopen(name, "w");
  
  Mat d;
  in_d.convertTo(d, CV_32F);
  //medianBlur(d, d, 3);
  
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
  ClifDataset *in_set = f_in.openDataset(0);
  
  Clif3DSubset *slice;
  slice = in_set->get3DSubset();
  
  int size[2];
  
  in_set->imgsize(size);
  
  double focal_length[2];
  
  in_set->getAttribute("calibration/intrinsics/default/projection", focal_length, 2);
  
  Mat epi;
  Mat depth = Mat::zeros(Size(size[0], size[1]), CV_64F);
  Mat score = Mat::zeros(Size(size[0], size[1]), CV_64F);
  for(int l=230/y_step*y_step;l<size[1];l+=y_step) {
    printf("line %d\n", l);
    for(double d=200;d<350;d+=d*0.0005) {
      slice->readEPI(epi, l, d, CLIF_DEMOSAIC, CV_INTER_LINEAR);
      //GaussianBlur(epi, epi, Size(1, 3), 0);
      //blur(epi, epi, Size(1, 7));
      score_epi(epi, score, depth, d, l);
    }
    for(double d=350;d<1000;d+=d*0.05) {
      slice->readEPI(epi, l, d, CLIF_DEMOSAIC, CV_INTER_LINEAR);
      //GaussianBlur(epi, epi, Size(1, 3), 0);
      //blur(epi, epi, Size(1, 7));
      score_epi(epi, score, depth, d, l);
    }
    if (l/y_step % 5 == 0) {
      Mat d16;
      depth.convertTo(d16, CV_16U);
      imwrite(out_name, d16);
      imwrite("out8bit.tif", depth*0.5);
      Mat img;
      readCvMat(in_set, in_set->imgCount()/2, img, CLIF_DEMOSAIC);
      write_obj_depth("points.ply", depth, focal_length, size[0], size[1], img, 230/y_step*y_step, l);
    }
  }

  return EXIT_SUCCESS;
}