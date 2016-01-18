#include "cliini.h"

#include <cstdlib>
#include <vector>

#include "H5Cpp.h"
#include "H5File.h"

#include "hdf5.hpp"

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

using namespace H5;

int main(const int argc, const char *argv[])
{
  cliini_args *args = cliini_parsopts(argc, argv, &group);

  cliini_arg *input = cliargs_get(args, "input");
  cliini_arg *output = cliargs_get(args, "output");
  cliini_arg *src = cliargs_get(args, "source");
  cliini_arg *dst = cliargs_get(args, "destination");
  cliini_arg *dims = cliargs_get(args, "dims");
  cliini_arg *convert_float = cliargs_get(args, "convert-float");
  //cliini_arg *chunks = cliargs_get(args, "destination");
  
  if (!args || cliargs_get(args, "help\n") || !input || !output) {
    cliini_help(&group);
    return EXIT_FAILURE;
  }
  
  hsize_t *size = new hsize_t[cliarg_sum(dims)];
  
  for(int i=0;i<cliarg_sum(dims);i++)
    size[i] = cliarg_nth_int(dims, cliarg_sum(dims)-i-1);
  
  H5File src_file(cliarg_str(input), H5F_ACC_RDONLY);
  H5File dst_file;
  
  try {
    dst_file.openFile(cliarg_str(output), H5F_ACC_RDWR);
  }
  catch (...)
  {
    dst_file = H5File(cliarg_str(output), H5F_ACC_TRUNC);
  }
  
  DataSet src_data = src_file.openDataSet(cliarg_str(src));
  
  DataSpace dataspace(cliarg_sum(dims), size);
  
  H5::DataType convert_type;
  
  convert_type = src_data.getDataType();
  
  if (convert_float)
    convert_type = H5::PredType::IEEE_F32LE;

  h5_create_path_groups(dst_file, boost::filesystem::path(cliarg_str(dst)).parent_path());
  
  if (h5_obj_exists(dst_file, cliarg_str(dst)))
    dst_file.unlink(cliarg_str(dst));
  
  DataSet dst_data = dst_file.createDataSet(cliarg_str(dst), convert_type, dataspace);
  
  void *buf = malloc(src_data.getInMemDataSize());
  
  src_data.read(buf, convert_type);
  dst_data.write(buf, convert_type);
  
  return EXIT_SUCCESS;
}