#include "cliini.h"

#include <cstdlib>
#include <vector>

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE //FIXME need portable extension matcher...
#endif
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef CLIF_COMPILER_MSVC
#include <unistd.h>
#endif

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
  
  if (!args || cliargs_get(args, "help\n") || !input) {
    printf("TODO: print help!");
    return EXIT_FAILURE;
  }

  string in_name(cliarg_nth_str(input,0));
  ClifFile f_in(in_name, H5F_ACC_RDONLY);
  Dataset *in_set = f_in.openDataset(0);
  
  //std::vector<cv::Mat> channels;
  Mat channels;
  
  for(int i=0;i<in_set->Datastore::count();i++)
    readCvMat(in_set, i, channels, UNDISTORT);

  return EXIT_SUCCESS;
}