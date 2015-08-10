#include "cliini.h"

#include <cstdlib>
#include <vector>

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
    -1, //argcount max
    CLIINI_STRING, //type
    0, //flags
    'i'
  },
  {
    "output",
    1, //argcount
    1, //argcount
    CLIINI_STRING, //type
    0, //flags
    'o'
  },
  {
    "config",
    1, //argcount
    1, //argcount
    CLIINI_STRING, //type
    0, //flags
    'c'
  },
  {
    "types",
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

int main(const int argc, const char *argv[])
{
  cliini_args *args = cliini_parsopts(argc, argv, &group);

  cliini_arg *input = cliargs_get(args, "input");
  cliini_arg *output = cliargs_get(args, "output");
  cliini_arg *config = cliargs_get(args, "config");
  cliini_arg *types = cliargs_get(args, "types");
  
  if (!args || cliargs_get(args, "help\n") || !input || !output || !config || !types) {
    printf("TODO: print help!");
    return EXIT_FAILURE;
  }
  
  Attributes attrs(cliarg_str(config),cliarg_str(types));
  
  H5File lffile(cliarg_str(output), H5F_ACC_TRUNC);
  
  Dataset dataset(lffile, "/clif/set1");
  
  dataset.setAttributes(attrs);
  dataset.writeAttributes();
  
  vector<char*> in_names(cliarg_sum(input));
  cliarg_strs(input, &in_names[0]);
  
  Mat img = imread(in_names[0], CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
  
  int w = img.size().width;
  int h = img.size().height;
  int depth = img.depth();
  
  Datastore imgs(&dataset, "data", w, h, cliarg_sum(input));
  
  //imgs.writeRawImage(0, img.data);
  for(int i=1;i<cliarg_sum(input);i++) {
    //Mat img = imread(in_names[i], CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
    assert(w == img.size().width);
    assert(h = img.size().height);
    assert(depth = img.depth());
    printf("store idx %d: %s\n", i, in_names[i]);
    //imgs.writeRawImage(i, img.data);
  }

  return EXIT_SUCCESS;
}