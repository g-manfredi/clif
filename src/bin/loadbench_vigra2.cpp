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

#include "clif_vigra.hpp"
#include "subset3d.hpp"

#include <vigra/imageinfo.hxx>
#include <vigra/impex.hxx>

using namespace clif;
using namespace std;
using namespace vigra;

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

template<typename T> class save_channel_functor {
public:
void operator()(int ch, FlexChannels<2> &img, const char *fmt = "%03d.tif")
{
  char buf[64];
  sprintf(buf, fmt, ch);
  
  MultiArrayView<2,T> view = img.channel<T>(ch);
  std::vector<MultiArrayView<2,T>> *views = img.channels<T>();
  
  exportImage(view, ImageExportInfo(buf));
}
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
  Subset3d *subset = new Subset3d(in_set);
  
  FlexChannels<2> img;

  //read first image from dataset
  readImage(in_set, 0, img);
  img.callChannels<save_channel_functor>(img, "view0_ch%d.tif");

  //read epi from line 50, dispatiry 1.5 px
  readEPI(subset, img, 50, 1.5);
  img.callChannels<save_channel_functor>(img, "epi50_ch%d.tif");
  
  return EXIT_SUCCESS;
}