#include "cliini.h"

#include <cstdlib>
#include <vector>

#include "H5Cpp.h"
#include "H5File.h"
#include "opencv2/highgui/highgui.hpp"

#include "clif.hpp"

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
  }
};

cliini_optgroup group = {
  opts,
  NULL,
  sizeof(opts)/sizeof(*opts),
  0
};

int main(const int argc, const char *argv[])
{
  
  cliini_args *args = cliini_parsopts(argc, argv, &group);
  
  if (!args) {
    printf("error parsing command line, exiting.\n");
    return EXIT_FAILURE;
  }
  
  if (cliarg_get(args, "help\n"))
    printf("TODO: print help!");
  
  cliini_arg *input = cliarg_get(args, "input");
  cliini_arg *output = cliarg_get(args, "output");
  if (input && output) {
    H5File lffile(cliarg_str(output), H5F_ACC_TRUNC);
    
    vector<char*> in_names(cliarg_sum(input));
    cliarg_strs(input, &in_names[0]);
    
    Mat img = imread(in_names[0], CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
    
    assert(img.depth() == CV_8U);
    assert(img.channels() == 1);
    
    //FIXME only fixed bayer RG (opencv: bg) pattern for now!
    Datastore lfdata(lffile, "/clif/set1", img.size().width, img.size().height, cliarg_sum(input), clif::DataType::UINT8, DataOrg::BAYER_2x2, DataOrder::RGGB);
    
    for(int i=0;i<cliarg_sum(input);i++)
      printf("input %d: %s\n", i, in_names[i]);
  }
  
  return EXIT_SUCCESS;
}