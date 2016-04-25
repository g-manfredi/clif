#include "cliini.h"

#include <cstdlib>
#include <vector>

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE //FIXME need portable extension matcher...
#endif
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif


#include <clif/dataset.hpp>
#include <clif/hdf5.hpp>
#include <clif/clif_cv.hpp>
#include <clif/calib.hpp>
#include <clif/cam.hpp>

#include <H5Library.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace clif;
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
    CLIINI_ARGCOUNT_ANY, //argcount max
    CLIINI_STRING, //type
    0, //flags
    'i',
    NULL,
    "files to import into clif dataset, supported types: clif files (*.clif), ini files (*.ini) and images (*.png, *.tiff, ...)",
    "[file1] [file2] ..."
  },
  {
    "output",
    1, //argcount
    CLIINI_ARGCOUNT_ANY, //argcount
    CLIINI_STRING, //type
    0, //flags
    'o',
    NULL,
    "pass single output clif file",
    "<file>"
  },
  {
    "cvt-gray",
    0, //argcount
    0, //argcount
    CLIINI_NONE, //type
    0, //flags
    0,
    NULL,
    "convert image to grayscale on load"
  },
  {
    "deep-copy",
    0, //argcount
    0, //argcount
    CLIINI_NONE, //type
    0, //flags
    0,
    NULL,
    "do full copy instead of shallow (external link)"
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
    "store",
    2, //argcount
    CLIINI_ARGCOUNT_ANY, //argcount
    CLIINI_STRING,
    0,
    's',
    NULL,
    "import images into [#dims]-dimensional store at [path]",
    "[path] [#dims] <img file1> <img file 2> ..."
  },
  {
    "store-dims",
    2, //argcount
    CLIINI_ARGCOUNT_ANY, //argcount
    CLIINI_STRING,
    0,
    0,
    NULL,
    "import images into [#dims]-dimensional store at [path] with dimensions [dim0]x[dim1]x..., first dimensions must be (in that order): image widht, height, #channels. Images will be appended at dim3 and on overflow the index will be counted up (so you can fill a multi-dimensional matrix with images).",
    "[path] [#dims] [dim0] [dim1] ... [dim#] <img file1> <img file 2> ..."
  },
  {
    "precalc-undist",
    3, //argcount
    3, //argcount
    CLIINI_INT,
    0,
    0,
    NULL,
    "precalculate undistortion/recticication maps",
    "[disp_start] [disp_stop] [step]"
  },
  {
    "include",
    1, //argcount
    CLIINI_ARGCOUNT_ANY, //argcount
    CLIINI_STRING
  },
  {
    "exclude",
    1, //argcount
    CLIINI_ARGCOUNT_ANY, //argcount
    CLIINI_STRING
  },
  {
    "detect-patterns",
    0,0
  },
  {
    "opencv-calibrate",
    0,0
  },
  {
    "ucalib-calibrate",
    0,0
  },
  {
    "gen-proxy-loess",
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

const char *clif_extension_pattern = ".clif";
const char *ini_extension_pattern = ".ini";
const char *mat_extension_pattern = ".mat";
//ksh extension match using FNM_EXTMATCH
#ifdef WIN32
  const char *img_extension_pattern = ".tif";
#else
  const char *img_extension_pattern = "*.+(png|tif|tiff|jpg|jpeg|jpe|jp2|bmp|dib|pbm|pgm|ppm|sr|ras|exr)";
#endif
  
vector<string> extract_matching_strings(cliini_arg *arg, const char *pattern)
{
  vector<string> files;
  
  for(int i=0;i<cliarg_sum(arg);i++) {
    //FIXME not working on windows!
	std::cout << boost::filesystem::extension(cliarg_nth_str(arg, i)) << "\n";
    if (!boost::filesystem::extension(cliarg_nth_str(arg, i)).compare(pattern))
      files.push_back(cliarg_nth_str(arg, i));
  }
    
  return files;
}

void errorexit(const char *msg)
{
  printf("ERROR: %s\n",msg);
  exit(EXIT_FAILURE);
}

inline bool file_exists(const std::string& name)
{
  struct stat st;   
  return (stat(name.c_str(), &st) == 0);
}

typedef unsigned int uint;

bool link_output = true;

int main(const int argc, const char *argv[])
{
  bool inplace = false;
  cliini_args *args = cliini_parsopts(argc, argv, &group);

  cliini_arg *input = cliargs_get(args, "input");
  cliini_arg *output = cliargs_get(args, "output");
  cliini_arg *types = cliargs_get(args, "types");
  cliini_arg *calib_imgs = cliargs_get(args, "calib-images");
  cliini_arg *include = cliargs_get(args, "include");
  cliini_arg *exclude = cliargs_get(args, "exclude");
  cliini_arg *stores = cliargs_get(args, "store");
  cliini_arg *dim_stores = cliargs_get(args, "store-dims");
  cliini_arg *cvt_gray = cliargs_get(args, "cvt-gray");
  cliini_arg *precalc_undist = cliargs_get(args, "precalc-undist");
  cliini_arg *deep_copy = cliargs_get(args, "deep-copy");
  
  if (deep_copy) {
    printf("deep copy!\n");
    link_output = false;
  }
  
  
  if (!args || cliargs_get(args, "help\n") || (!input && !calib_imgs && !stores) || !output) {
    cliini_help(&group);
    return EXIT_FAILURE;
  }
  
  //several modes:                  input      output
  //add/append data to clif file     N x *     1x clif
  //extract image/and or ini        1 x clif  1x init/ Nx img pattern
  
  vector<string> clif_append = extract_matching_strings(output, clif_extension_pattern);
  vector<string> clif_extract_images = extract_matching_strings(output, img_extension_pattern);
  vector<string> clif_extract_attributes = extract_matching_strings(output, ini_extension_pattern);
  
  vector<string> input_clifs = extract_matching_strings(input, clif_extension_pattern);
  vector<string> input_imgs  = extract_matching_strings(input, img_extension_pattern);
  vector<string> input_inis  = extract_matching_strings(input, ini_extension_pattern);
  //vector<string> input_mats  = extract_matching_strings(input, mat_extension_pattern);
  vector<string> input_calib_imgs = extract_matching_strings(calib_imgs, img_extension_pattern);
  
  /*if (input_mats.size()) {
    for(int i=0;i<input_mats.size();i++)
      list_mat(input_mats[i]);
    return EXIT_SUCCESS;
  }*/
  
  bool output_clif;
  //std::string input_set_name;
  std::string output_set_name("default");
  
  if (clif_append.size()) {
    if (clif_append.size() > 1)
      errorexit("only a single output clif file may be specififed!");
    if (clif_extract_images.size() || clif_extract_attributes.size())
      errorexit("may not write to clif at the same time as extracting imgs/attributes!");
    output_clif = true;
  }
  else {
    if (!clif_extract_images.size() && !clif_extract_attributes.size())
      errorexit("no valid output format found!");
    if (input_clifs.size() != 1)
      errorexit("only single input clif file allowed!");
    output_clif = false;
  }
  
  if (output_clif && input_clifs.size() && !clif_append[0].compare(input_clifs[0]))
    inplace = true;
  
  if (!output_clif && (stores || dim_stores))
    errorexit("can only add datastores to clif output!");
  
  if (output_clif) {
    ClifFile f_out;
    
    if (file_exists(clif_append[0])) {
      f_out.open(clif_append[0], H5F_ACC_RDWR);
    }
    else
      f_out.create(clif_append[0]);
    
    Dataset *set;
    //FIXME multiple dataset handling!
    if (f_out.datasetCount()) {
      printf("INFO: appending to HDF5 DataSet %s\n", f_out.datasetList()[0].c_str());
      set = f_out.openDataset(0);
    }
    else {
      printf("INFO: creating new HDF5 DataSet %s\n", output_set_name.c_str());
      set = f_out.createDataset(output_set_name);
    }
    
    if (!inplace)
    for(uint i=0;i<input_clifs.size();i++) {
      ClifFile f_in(input_clifs[i], H5F_ACC_RDWR);
      ///FIXME for linking!
      //ClifFile f_in(input_clifs[i], H5F_ACC_RDONLY);
      
      //FIXME input name handling/selection
      if (f_in.datasetCount() != 1)
        errorexit("FIXME: at the moment only files with a single dataset are supported by this program.");
      
      //FIXME implement dataset handling for datasets without datastore!
      //TODO check: is ^ already working?
      Dataset *in_set = f_in.openDataset(0);
      printf("open input refs; %d\n", H5Fget_obj_count(f_in.f.getId(), H5F_OBJ_ALL));
      if (include)
        for(int i=0;i<in_set->Attributes::count();i++) {
          bool match_found = false;
          Attribute *a = in_set->get(i);
          for(int j=0;j<cliarg_sum(include);j++)
            if (!fnmatch(cliarg_nth_str(include, j), a->name.generic_string().c_str(), FNM_PATHNAME)) {
              printf("include attribute: %s\n", a->name.generic_string().c_str());
              match_found = true;
              break;
            }
          if (match_found)
            set->append(a);
        }
      else if (exclude)
        for(int i=0;i<in_set->Attributes::count();i++) {
          bool match_found = false;
          Attribute *a = in_set->get(i);
          for(int j=0;j<cliarg_sum(exclude);j++)
            if (!fnmatch(cliarg_nth_str(exclude, j), a->name.generic_string().c_str(), FNM_PATHNAME)) {
              printf("exclude attribute: %s\n", a->name.c_str());
              match_found = true;
              break;
            }
          if (!match_found)
            set->append(a);
        }
      else {
        if (!link_output)
          set->append(in_set);
        else {
          printf("create dataset by linking!\n");
          set->link(f_out, in_set);
          
          delete set;
          f_out.close();

          delete in_set;
          in_set = NULL;
          f_in.close();
          f_out.open(clif_append[0], H5F_ACC_RDWR);
          set = f_out.openDataset();
        }
      }
      
      if (link_output && (include || exclude))
        printf("FIXME filtering not yet supported together with linking!\n");
      
      if (!link_output) {
        vector<string> h5datasets = listH5Datasets(f_in.f, in_set->path().generic_string());
        for(uint j=0;j<h5datasets.size();j++) {
          cout << "copy dataset" << h5datasets[j] << endl;
          //FIXME handle dataset selection properly!
          if (!h5_obj_exists(f_out.f, h5datasets[j])) {
            h5_create_path_groups(f_out.f, cpath(h5datasets[j]).parent_path());
            f_out.f.flush(H5F_SCOPE_GLOBAL);
            H5Ocopy(f_in.f.getId(), h5datasets[j].c_str(), f_out.f.getId(), h5datasets[j].c_str(), H5P_DEFAULT, H5P_DEFAULT);
          }
          else
            printf("TODO: dataset %s already exists in output, skipping!\n", h5datasets[j].c_str());
        }
        
        delete set;
        //FIXME dataset idx!
        set = f_out.openDataset(0);
        
        delete in_set;
      }
    }
    
    for(uint i=0;i<input_inis.size();i++) {
      printf("append ini file!\n");
      Attributes others;
      //FIXME append types (not replace!)
      if (types)
        others.open(input_inis[i].c_str(), cliarg_str(types));
      else
        others.open(input_inis[i].c_str());
      set->append(others);
    }
    
    if (stores) {
      int sum = 0;
      for(uint i=0;i<cliarg_inst_count(stores);i++) {
        int count = cliarg_inst_arg_count(stores, i);
        char *store_path = cliarg_nth_str(stores, sum);
        int dim = atoi(cliarg_nth_str(stores, sum+1));
        sum += 2;
        if (dim < 3) {
          printf("ERROR: datastore %s needs at least 4 dimensions, but dimension was %d (%s)\n", store_path, dim, cliarg_nth_str(stores, sum));
          exit(EXIT_FAILURE);
        }
        printf("add store %s\n", store_path);
        Datastore *store = set->addStore(store_path, dim);
#pragma omp parallel for ordered schedule(dynamic)
        for(int j=sum;j<cliarg_inst_arg_count(stores, i);j++) {
          printf("%s\n", cliarg_nth_str(stores, j));
          cv::Mat img = imread(cliarg_nth_str(stores, j), CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
          if (img.channels() > 1 && cvt_gray)
            cv::cvtColor(img, img, CV_BGR2GRAY);
          else if (img.channels() == 3)
            cvtColor(img, img, COLOR_BGR2RGB);
#pragma omp ordered
          store->append(clif::Mat3d(img));
        }
      }
    }
    
    if (dim_stores) {
      int sum = 0;
      for(uint i=0;i<cliarg_inst_count(dim_stores);i++) {
        int count = cliarg_inst_arg_count(dim_stores, i);
        char *store_path = cliarg_nth_str(dim_stores, sum);
        int dim = atoi(cliarg_nth_str(dim_stores, sum+1));
        sum += 2;
        printf("dim store %d!\n", dim);
        if (dim < 3) {
          printf("ERROR: datastore %s needs at least 4 dimensions, but dimension was %d (%s)\n", store_path, dim, cliarg_nth_str(dim_stores, sum));
          exit(EXIT_FAILURE);
        }
        Idx size(dim);
        for(int n=0;n<dim;n++,sum++)
          size[n] = atoi(cliarg_nth_str(dim_stores, sum));
        printf("add store %s\n", store_path);
        Datastore *store = set->addStore(store_path, dim);
#pragma omp parallel for schedule(dynamic)
        for(int j=sum;j<cliarg_inst_arg_count(dim_stores, i);j++) {
          Idx pos(dim);
        
          printf("%s\n", cliarg_nth_str(dim_stores, j));
          cv::Mat img = imread(cliarg_nth_str(dim_stores, j), CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
          if (img.channels() > 1 && cvt_gray)
            cv::cvtColor(img, img, CV_BGR2GRAY);
          else if (img.channels() == 3)
            cvtColor(img, img, COLOR_BGR2RGB);
#pragma omp critical
          store->write(clif::Mat3d(img), pos);
          int currdim = 3;
          pos[currdim] = j-sum;
          while(size[currdim] > 0 && pos[currdim] >= size[currdim] && currdim < dim-1) {
            pos[currdim] = 0;
            currdim++;
            pos[currdim]++;
          }
        }
      }
    }
    
    //FIXME allow "empty" datasets with only attributes!
    //FIXME handle appending to datastore!
    //FIXME check wether image format was sufficiently defined!
    //FIXME check wether image format was sufficiently defined!
    //FIXME how do we handle overwriting of data?
    
    //set->writeAttributes();
    
    //set dims for 3d LG (imgs are 3d themselves...)
    
    Datastore *store = set->getStore("data");
#pragma omp parallel for ordered schedule(dynamic)
    for(uint i=0;i<input_imgs.size();i++) {
      printf("store idx %d: %s\n", i, input_imgs[i].c_str());
      cv::Mat img = imread(input_imgs[i], CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
      assert(img.size().width && img.size().height);
      if (img.channels() > 1 && cvt_gray)
        cv::cvtColor(img, img, CV_BGR2GRAY);
      else if (img.channels() == 3)
        cvtColor(img, img, COLOR_BGR2RGB);
#pragma omp ordered
      store->appendImage(&img);
    }
    
    if (cliargs_get(args, "detect-patterns")) {
      pattern_detect(set);
      set->writeAttributes();
    }
    if (cliargs_get(args, "gen-proxy-loess")) {
      generate_proxy_loess(set, 33, 25);
      set->writeAttributes();
    }
    if (cliargs_get(args, "opencv-calibrate")) {
      opencv_calibrate(set);
      set->writeAttributes();
    }
    if (cliargs_get(args, "ucalib-calibrate")) {
      ucalib_calibrate(set);
      set->writeAttributes();
    }
    if (precalc_undist) {
      precalc_undists_maps(set, cliarg_nth_int(precalc_undist, 0), cliarg_nth_int(precalc_undist, 1), cliarg_nth_int(precalc_undist, 2));
    }
    delete set;
  }
  else {
    
    for(uint i=0;i<clif_extract_attributes.size();i++) {
      ClifFile f_in(input_clifs[0], H5F_ACC_RDONLY);
      
      //FIXME input name handling/selection
      if (f_in.datasetCount() != 1)
        errorexit("FIXME: at the moment only files with a single dataset are supported by this program.");
      
      //FIXME implement dataset handling for datasets without datastore!
        Dataset *in_set = f_in.openDataset(0);
        in_set->writeIni(clif_extract_attributes[i]);
        delete in_set;
    }
    
    for(uint i=0;i<clif_extract_images.size();i++) {
      ClifFile f_in(input_clifs[0], H5F_ACC_RDONLY);
      
      //FIXME input name handling/selection
      if (f_in.datasetCount() != 1)
        errorexit("FIXME: at the moment only files with a single dataset are supported by this program.");
      
      Dataset *in_set = f_in.openDataset(0);
      Datastore *in_imgs = in_set->getStore("data");
        
#pragma omp parallel for schedule(dynamic)
      for(int c=0;c<in_imgs->imgCount();c++) {
        char buf[4096];
        cv::Mat img;
        sprintf(buf, clif_extract_images[i].c_str(), c);
#pragma omp critical
        printf("store idx %d: %s\n", c, buf);
        std::vector<int> idx(in_imgs->dims(), 0);
        idx[3] = c;
        in_imgs->readImage(idx, &img);
        clifMat2cv(&img, &img);
        cvtColor(img, img, COLOR_RGB2BGR);
        imwrite(buf, img);
      }
      delete in_set;
    }
  }

  return EXIT_SUCCESS;
}
