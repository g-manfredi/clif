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

float scale = 1.0;

int x_step = 1;
int y_step = 1;

int minx = 350*scale;
int maxx = 1150*scale;

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

static inline double score_range(Mat &epi, int x, int y_start, int y_end, int c, double *weight_lu)
{
  double score = 0;
  double d;
  
  Vec3i avg(0,0,0);
  for(int j=y_start;j<=y_end;j++)
    avg += epi.at<Vec3us>(j,x);
  
  avg *= 1.0/(y_end-y_start+1);

  for(int j=y_start;j<=y_end;j++) {
    d = (1.0/(abs(c-j)+1));
    score += norm(avg-(Vec3i)epi.at<Vec3us>(j,x), NORM_L1)*d;// *weight_lu[j];
  }
  
  for(int j=y_start;j<=y_end-1;j++){
    d = (1.0/(abs(c-j)+1));
    //score += diffsum(epi.at<Vec3us>(j,x), epi.at<Vec3us>(j+1,x))*weight_lu[j];
    score += diffsum(epi.at<Vec3us>(j,x), epi.at<Vec3us>(j+1,x))*d;// *weight_lu[j];
  }
  return score;
}

void score_epi(Mat &epi, Mat &score_m, Mat &depth_m, double d, int l)
{
  int h = 81;//epi.size().height;
  
  double weight_lu[h];
  
  for(int i=0;i<h;i++) {
    double d = h/2-i;
    d = 0.5*0.5*d*d/(h*h*0.2*0.2);
    weight_lu[i] = exp(-d);
  }
  
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
    
    score = score_range(epi, i, 0, h/2,h/2,weight_lu)*(2.0/h);
    
    if (score < score_m.at<double>(l,i)) {
      score_m.at<double>(l,i) = score;
      depth_m.at<double>(l,i) = d;
    }
    
    score = score_range(epi, i, h/2, h-1,h/2,weight_lu)*(2.0/h);
    
    if (score < score_m.at<double>(l,i)) {
      score_m.at<double>(l,i) = score;
      depth_m.at<double>(l,i) = d;
    }
    
    score = score_range(epi, i, 0, h-1,h/2,weight_lu)*(1.0/(2.0*h));
    
    if (score < score_m.at<double>(l,i)) {
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


void structure_tensor_depth(Mat &epi, Mat &score_m, Mat &depth_m, double d, int l, double f, Subset3d *subset)
{
  vector<Mat> channels;
  
  Mat dx,dy;
  
  split(epi, channels);
  
  for(int c=1;c<2/*channels.size()*/;c++) {
    Mat channel = channels[c];
    
    Scharr(channel, dx, CV_32F, 1, 0);
    Scharr(channel, dy, CV_32F, 0, 1);
    
    dx *= 1.0/131072;
    dy *= 1.0/131072;
    Mat dxy;
    
    multiply(dx,dy,dxy);
    multiply(dx,dx,dx);
    multiply(dy,dy,dy);
    
    GaussianBlur(dx, dx, Size(5,11), 0);
    GaussianBlur(dy, dy, Size(5,11), 0);
    GaussianBlur(dxy, dxy, Size(5,11), 0);
    
    /*imwrite("dx.tif", dx*256);
    imwrite("dy.tif", dy*256);
    imwrite("dxy.tif", dxy*256);
    
    exit(0);*/
    if (l == 500)
      imwrite("epi.tif", epi);
    
    int j = epi.size().height/2;
    for(int i=1;i<epi.size().width-1;i++) {
      double sxx = dx.at<float>(j, i);
      double syy = dy.at<float>(j, i);
      double sxy = dxy.at<float>(j, i);
      double d_y2_x2 = syy-sxx;
      double xy2 = sxy*sxy;
      double a_y2_x2 = syy+sxx;
      double disp = d+tan(0.5*atan2((syy-sxx),(2*sxy)));
      
      
      /*z = -f*s/x;
      
      -x/s+d = f/z
      
      z = -f/(x/s+d);*/
      
      //double depth = -f/(x/s+d); //-f*s/x;5
      double coherence = ((syy-sxx)*(syy-sxx)+4.0*xy2)/(a_y2_x2*a_y2_x2);
      
      //printf("coherence: %f disp %f\n", coherence, disp);
      //printf("%f\n", subset->disparity2depth(disp, scale));
      
      
      if (coherence > score_m.at<double>(l,i)) {
        score_m.at<double>(l,i) = coherence;
        depth_m.at<double>(l,i) = subset->disparity2depth(disp, scale);
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
  
  in_set->imgsize(size);
  
  size[0] *= scale;
  size[1] *= scale;
  
  double focal_length[2];
  
  in_set->getAttribute("calibration/intrinsics/checkers/projection", focal_length, 2);
  
  focal_length[0] *= scale;
  focal_length[1] *= scale;
  
  
  Mat img;
  readCvMat(in_set, in_set->Datastore::count()/2, img, UNDISTORT, scale);
  
  Mat epi;
  Mat depth = Mat::zeros(Size(size[0], size[1]), CV_64F);
  //Mat score(Size(size[0], size[1]), CV_64F, Scalar::all(std::numeric_limits<double>::max()));
  Mat score = Mat::zeros(Size(size[0], size[1]), CV_64F);
  for(int l=500/y_step*y_step;l<size[1];l+=y_step) {
    printf("line %d\n", l);
    //double d = 3;
    for(double d=5;d>=5;d-=2) {
      slice->readEPI(epi, l, d, ClifUnit::PIXELS, UNDISTORT, CV_INTER_LINEAR, scale);
      GaussianBlur(epi, epi, Size(3, 3), 0);
      //blur(epi, epi, Size(1, 7));
      //score_epi(epi, score, depth, d, l);
      printf("depth %f\n", slice->disparity2depth(d,scale));
      structure_tensor_depth(epi, score, depth, d, l, (focal_length[0]+focal_length[1])/2, slice);
    }
    /*for(double d=350;d<1000;d+=d*0.05) {
      slice->readEPI(epi, l, d, CLIF_UNDISTORT, CV_INTER_LINEAR);
      GaussianBlur(epi, epi, Size(1, 3), 0);
      //blur(epi, epi, Size(1, 7));
      score_epi(epi, score, depth, d, l);
    }*/
  //}
    if (l/y_step % 400 == 0) {
      Mat d16;
      depth.convertTo(d16, CV_16U);
      //resize(d16, d16, Size(depth.size().width/x_step, depth.size().height/y_step), cv::INTER_NEAREST);
      imwrite(out_name, d16);
      imwrite("out8bit.tif", depth*0.25);
      
      //resize(depth, depth, Size(depth.size().x/x_step, depth.size().y/y_step), CV_INTER_NEAREST);
      
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
      //depth.convertTo(fmat, CV_32F);
      //medianBlur(fmat, med, 7);
     // write_ply_depth("points_m7.ply", med, focal_length, size[0], size[1], img, 230/y_step*y_step, l);
    }
  }

  return EXIT_SUCCESS;
}