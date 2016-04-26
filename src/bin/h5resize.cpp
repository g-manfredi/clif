#include "cliini.h"

#include <cstdlib>
#include <vector>

#include <clif/dataset.hpp>

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
  },
  {
    "dims",
    1, //argcount min
    CLIINI_ARGCOUNT_ANY, //argcount max
    CLIINI_INT, //type
    0, //flags
    'r'
  },
  {
    "chunksize",
    1, //argcount min
    CLIINI_ARGCOUNT_ANY, //argcount max
    CLIINI_INT, //type
    0, //flags
    'c'
  },
  {
    "convert-float",
    0,
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
  
  Mat m_in, m_out;
  in_store->read(m_in);
  
  m_out.create(m_in.type(), {m_in[0]/2, m_in[1]/2, m_in.r(2,m_in.size()-1)});
  int i = 0;
  for(auto pos :  Idx_It_Dims(m_in, 2, m_in.size()-1)) {
    printf("scale img %d\n", i++);
    Mat in_bound = m_in;
    Mat out_bound = m_out;
    cv::Mat tmp;
    
    for(int dim=m_in.size()-1;dim>=2;dim--)
      in_bound = in_bound.bind(dim, pos[dim]);
    
    for(int dim=m_in.size()-1;dim>=2;dim--)
      out_bound = out_bound.bind(dim, pos[dim]);
    
    cv::GaussianBlur(cvMat(in_bound), tmp, cv::Size(3,3), 0.5, 0.5);
    cv::resize(tmp, cvMat(out_bound), cv::Size(0,0), 0.5, 0.5, cv::INTER_NEAREST);
  }

  printf("write!\n");
  out_store->write(m_out);
  
  out_store->flush();
  
  return EXIT_SUCCESS;
}
