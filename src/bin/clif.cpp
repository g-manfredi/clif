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
    CLIINI_ARGCOUNT_ANY, //argcount max
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
  }
};

cliini_optgroup group = {
  opts,
  NULL,
  sizeof(opts)/sizeof(*opts),
  0,
  0
};

const char *clif_extension_pattern = "*.cli?";
const char *ini_extension_pattern = "*.ini";
//ksh extension match using FNM_EXTMATCH
const char *img_extension_pattern = "*.+(png|tif|tiff|jpg|jpeg|jpe|jp2|bmp|dib|pbm|pgm|ppm|sr|ras)";

vector<string> extract_matching_strings(cliini_arg *arg, const char *pattern)
{
  vector<string> files;
  
  for(int i=0;i<cliarg_sum(arg);i++)
    if (!fnmatch(pattern, cliarg_nth_str(arg, i), FNM_CASEFOLD | FNM_EXTMATCH))
      files.push_back(cliarg_nth_str(arg, i));
    
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

int main(const int argc, const char *argv[])
{
  cliini_args *args = cliini_parsopts(argc, argv, &group);

  cliini_arg *input = cliargs_get(args, "input");
  cliini_arg *output = cliargs_get(args, "output");
  cliini_arg *types = cliargs_get(args, "types");
  
  if (!args || cliargs_get(args, "help\n") || !input || !output) {
    printf("TODO: print help!");
    return EXIT_FAILURE;
  }
  
  //several modes:                  input      output
  //add/append data to clif file     N x *     1x clif
  //extract image/and or ini        1 x clif  1x init/ Nx img pattern
  
  vector<string> clif_append = extract_matching_strings(output, clif_extension_pattern);
  vector<string> clif_extra_images = extract_matching_strings(output, img_extension_pattern);
  vector<string> clif_extract_attributes = extract_matching_strings(output, ini_extension_pattern);
  
  vector<string> input_clifs = extract_matching_strings(input, clif_extension_pattern);
  vector<string> input_imgs  = extract_matching_strings(input, img_extension_pattern);
  vector<string> input_inis  = extract_matching_strings(input, ini_extension_pattern);
  
  bool output_clif;
  //std::string input_set_name;
  std::string output_set_name("default");
  
  if (clif_append.size()) {
    if (!types)
      errorexit("FIXME add global type declaration!");
    if (clif_append.size() > 1)
      errorexit("only a single output clif file may be specififed!");
    if (clif_extra_images.size() || clif_extract_attributes.size())
      errorexit("may not write to clif at the same time as extracting imgs/attributes!");
    output_clif = true;
  }
  else {
    if (!clif_extra_images.size() && !clif_extract_attributes.size())
      errorexit("no valid output format found!");
    if (input_clifs.size() != 1)
      errorexit("only single input clif file allowed!");
    output_clif = false;
  }
  
  if (output_clif) {
    CvClifFile f_out;
    
    if (file_exists(clif_append[0])) {
      f_out.open(clif_append[0], H5F_ACC_RDWR);
    }
    else
      f_out.create(clif_append[0]);
    
    CvClifDataset set;
    //FIXME multiple dataset handling!
    if (f_out.datasetCount()) {
      printf("INFO: appending to HDF5 DataSet %s\n", f_out.datasetList()[0].c_str());
      set = f_out.openDataset(0);
    }
    else {
      printf("INFO: creating new HDF5 DataSet %s\n", output_set_name.c_str());
      set = f_out.createDataset(output_set_name);
    }
    
    
    
    for(int i=0;i<input_clifs.size();i++) {
      ClifFile f_in(input_clifs[i], H5F_ACC_RDONLY);
      
      //FIXME input name handling/selection
      if (f_in.datasetCount() != 1)
        errorexit("FIXME: at the moment only files with a single dataset are supported by this program.");
      
      //FIXME implement dataset handling for datasets without datastore!
      ClifDataset set = f_in.openDataset(0);
      set.append(static_cast<Attributes&>(set));
      
      printf("FIXME: append image data/other datasets!");
    }
    
    for(int i=0;i<input_inis.size();i++) {
      printf("append ini file!\n");
      //FIXME multiple type files?
      Attributes others = Attributes(input_inis[i].c_str(), cliarg_str(types));
      set.append(others);
    }
    
    //FIXME allow "empty" datasets with only attributes!
    //FIXME handle appending to datastore!
    //FIXME check wether image format was sufficiently defined!
    //FIXME check wether image format was sufficiently defined!
    //FIXME how do we handle overwriting of data?
    
    set.writeAttributes();
    
    for(int i=0;i<input_imgs.size();i++) {
      printf("store idx %d: %s\n", i, input_imgs[i].c_str());
      Mat img = imread(input_imgs[i], CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
      int w = img.size().width;
      int h = img.size().height;
      set.appendRawImage(w, h, img.data);
    }
  }
  else {
    
    for(int i=0;i<clif_extract_attributes.size();i++) {
      ClifFile f_in(input_clifs[0], H5F_ACC_RDONLY);
      
      //FIXME input name handling/selection
      if (f_in.datasetCount() != 1)
        errorexit("FIXME: at the moment only files with a single dataset are supported by this program.");
      
      //FIXME implement dataset handling for datasets without datastore!
      ClifDataset set = f_in.openDataset(0);
        set.writeIni(clif_extract_attributes[i]);
    }
    
    for(int i=0;i<clif_extra_images.size();i++) {
      CvClifFile f_in(input_clifs[0], H5F_ACC_RDONLY);
      
      //FIXME input name handling/selection
      if (f_in.datasetCount() != 1)
        errorexit("FIXME: at the moment only files with a single dataset are supported by this program.");
      
      //FIXME implement dataset handling for datasets without datastore!
      CvClifDataset set = f_in.openDataset(0);
        
      char buf[4096];
      for(int c=0;c<set.imgCount();c++) {
        Mat img;
        sprintf(buf, clif_extra_images[i].c_str(), c);
        printf("store idx %d: %s\n", c, buf);
        set.readCvMat(c, img);
        imwrite(buf, img);
      }
    }
  }

  return EXIT_SUCCESS;
}