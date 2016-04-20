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

#include <clif/clif_cv.hpp>
#include <clif/calib.hpp>

#include <H5Cpp.h>
#include <H5File.h>


#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>


using namespace clif;
using namespace std;
using namespace cv;
using boost::filesystem::path;
using H5::H5File;

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
    "attribute",
    1, //argcount min
    1, //argcount max
    CLIINI_STRING, //type
    0, //flags
    'a'
  },
  {
    "output",
    1, //argcount
    1, //argcount
    CLIINI_STRING, //type
    0, //flags
    'o'
  }
};

cliini_optgroup group = {
  opts,
  NULL,
  sizeof(opts)/sizeof(*opts),
  0,
  0
};

void errorexit(const char *msg)
{
  printf("ERROR: %s\n",msg);
  exit(EXIT_FAILURE);
}

int main(const int argc, const char *argv[])
{/*
  cliini_args *args = cliini_parsopts(argc, argv, &group);

  cliini_arg *input = cliargs_get(args, "input");
  cliini_arg *output = cliargs_get(args, "output");
  cliini_arg *attribute = cliargs_get(args, "attribute");
  
  if (!args || cliargs_get(args, "help\n") || !input || !output || !attribute) {
    printf("TODO: print help!");
    return EXIT_FAILURE;
  }
  
  ClifFile f_out;
  
  if (file_exists(cliarg_str(output))) {
    f_out.open(cliarg_str(output), H5F_ACC_RDWR);
  }
  else
    f_out.create(cliarg_str(output));
  
  Mat img = imread(cliarg_str(input));
  
  
    */

  return EXIT_SUCCESS;
}
