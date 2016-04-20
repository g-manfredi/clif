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

#include <H5Cpp.h>
#include <H5File.h>

#include <clif/dataset.hpp>
#include <clif/clif.hpp>

#include <opencv2/highgui/highgui.hpp>


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

typedef unsigned int uint;

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
{
  cliini_args *args = cliini_parsopts(argc, argv, &group);

  cliini_arg *input = cliargs_get(args, "input");
  
  if (!input || cliarg_sum(input) != 1)
    errorexit("need exactly on input file");
  
  string name(cliarg_str(input));
  
  ClifFile f(name, H5F_ACC_RDONLY);
  
  vector<string> datasets = f.datasetList();
  
  for(uint i=0;i<datasets.size();i++) {
    Dataset *set = f.openDataset(i);
    
    printf("Found dataset \"%s\" at index %d:\n", datasets[i].c_str(), i);

    set->getTree().print(10);
    delete set;
  }
  

  return EXIT_SUCCESS;
}
