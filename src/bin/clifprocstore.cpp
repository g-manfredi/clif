#include "cliini.h"

#include <cstdlib>
#include <vector>

#include "clif/dataset.hpp"
#include "clif/clif.hpp"

#include <opencv2/imgproc/imgproc.hpp>

using namespace clif;
using namespace std;
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
  }
};

cliini_optgroup group = {
  opts,
  NULL,
  sizeof(opts)/sizeof(*opts),
  0,
  0
};

using namespace clif;

int main(const int argc, const char *argv[])
{
  cliini_args *args = cliini_parsopts(argc, argv, &group);

  cliini_arg *input = cliargs_get(args, "input");
  cliini_arg *output = cliargs_get(args, "output");
  cliini_arg *src = cliargs_get(args, "source");
  cliini_arg *dst = cliargs_get(args, "destination");
  
  if (!args || cliargs_get(args, "help\n") || !input || !output) {
    cliini_help(&group);
    return EXIT_FAILURE;
  }
  
  ClifFile f_in(cliarg_str(input), H5F_ACC_RDONLY);
  ClifFile f_out;
  
  
  Dataset *in_set = f_in.openDataset(0);
  Datastore *in_store = in_set->getStore(cliarg_str(src));
  
  f_out.create(cliarg_str(output));
  Dataset *out_set = f_out.createDataset();
  
  Datastore *out_store = out_set->addStore(cliarg_str(dst));
  
  ProcData proc;
  proc.set_flags(UNDISTORT);
  proc.set_scale(0.5);
  
  Mat m_out;
  in_store->read(m_out, proc);
  
  out_store->write(m_out);
  
  out_store->flush();
  
  return EXIT_SUCCESS;
}
