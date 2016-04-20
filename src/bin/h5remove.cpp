#include "cliini.h"

#include <cstdlib>
#include <vector>

#include <H5Cpp.h>
#include <H5File.h>

#include <clif/hdf5.hpp>

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
  }
};

cliini_optgroup group = {
  opts,
  NULL,
  sizeof(opts)/sizeof(*opts),
  0,
  0
};

using namespace H5;

int main(const int argc, const char *argv[])
{
  cliini_args *args = cliini_parsopts(argc, argv, &group);

  cliini_arg *input = cliargs_get(args, "input");
  cliini_arg *src = cliargs_get(args, "source");
  
  if (!args || cliargs_get(args, "help\n") || !input) {
    cliini_help(&group);
    return EXIT_FAILURE;
  }
  
  
  H5File src_file(cliarg_str(input), H5F_ACC_RDWR);
  src_file.unlink(cliarg_str(src));
  
  return EXIT_SUCCESS;
}
