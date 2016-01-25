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
#include "opencv2/calib3d/calib3d.hpp"

#include "clif_cv.hpp"
#include "calib.hpp"
#include "dataset.hpp"

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
    "source",
    1, //argcount min
    1, //argcount max
    CLIINI_STRING, //type
    0, //flags
    's'
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
    "destination",
    1, //argcount min
    1, //argcount max
    CLIINI_STRING, //type
    0, //flags
    'd'
  },
  {
    "dims",
    1, //argcount min
    CLIINI_ARGCOUNT_ANY, //argcount max
    CLIINI_INT, //type
    0, //flags
    'r'
  }
  {
    "chunksize",
    1, //argcount min
    CLIINI_ARGCOUNT_ANY, //argcount max
    CLIINI_INT, //type
    0, //flags
    'c'
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
  cliini_arg *src = cliargs_get(args, "source");
  cliini_arg *dst = cliargs_get(args, "destination");
  
  if (!args || cliargs_get(args, "help\n") || !input || !output) {
    printf("TODO: print help!");
    return EXIT_FAILURE;
  }
  
  
  H5File src_file(cliarg_str(input), H5F_ACC_RDONLY);
  H5File dst_file(cliarg_str(input), H5F_ACC_RDONLY);
  
  DataSet dataset = file.openDataSet( DATASET_NAME );
  
  ClifFile f_in;
  f_in.open(cliarg_str(input), H5F_ACC_RDONLY);
  Dataset *in_set = f_in.openDataset(0);
  
  printf("output %s\n", cliarg_str(output));
  
  H5::H5File f_out(cliarg_str(output), H5F_ACC_TRUNC);
  Dataset out_set;
  
  out_set.link(f_out, in_set);
  
  out_set.writeAttributes();
  
  return EXIT_SUCCESS;
}