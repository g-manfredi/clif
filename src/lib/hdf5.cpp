#include "hdf5.hpp"

#include <H5File.h>
#include <H5FaccProp.h>
#include <H5FcreatProp.h>

#include "enumtypes.hpp"
#ifdef CLIF_COMPILER_MSVC
#include "io.h"
#include "Windows.h"
#endif



using namespace H5;

typedef unsigned int uint;

namespace clif {

bool h5_obj_exists(H5::H5File f, const char * const path)
{
  H5E_auto2_t  oldfunc;
  void *old_client_data;
  
  H5Eget_auto(H5E_DEFAULT, &oldfunc, &old_client_data);
  H5Eset_auto(H5E_DEFAULT, NULL, NULL);
  
  int status = H5Gget_objinfo(f.getId(), path, 0, NULL);
  
  H5Eset_auto(H5E_DEFAULT, oldfunc, old_client_data);
  
  if (status == 0)
    return true;
  return false;
}

bool h5_obj_exists(H5::H5File f, const std::string path)
{
  return h5_obj_exists(f,path.c_str());
}

bool h5_obj_exists(H5::H5File f, const boost::filesystem::path path)
{
  return h5_obj_exists(f, path.generic_string().c_str());
}
  
static void datasetlist_append_group(std::vector<std::string> &list, H5::Group &g,  std::string group_path)
{    
  for(uint i=0;i<g.getNumObjs();i++) {
    H5G_obj_t type = g.getObjTypeByIdx(i);
    
    std::string name = appendToPath(group_path, g.getObjnameByIdx(hsize_t(i)));
    
    if (type == H5G_GROUP) {
      H5::Group sub = g.openGroup(g.getObjnameByIdx(hsize_t(i)));
      datasetlist_append_group(list, sub, name);
    }
    else if (type == H5G_DATASET)
      list.push_back(name);
  }
}

std::vector<std::string> listH5Datasets(H5::H5File &f, std::string parent)
{
  std::vector<std::string> list;
  H5::Group group = f.openGroup(parent.c_str());
  
  datasetlist_append_group(list, group, parent);
  
  return list;
}
  
ClifFile h5_memory_file()
{
  ClifFile cf;
  
  FileAccPropList acc_plist;
  acc_plist.setCore(16 * 1024, false);
#ifdef CLIF_COMPILER_MSVC
  char *tmppath = (char*)malloc(1024);
  char *tmppath2 = (char*)malloc(1024);
  if (!GetTempPath(1024, tmppath))
	  abort();
  strcat(tmppath, "openlfhdf5tempfile_%d_XXXXXX");
  //increase number of temporary files...
  sprintf(tmppath2, tmppath, rand());
  char *tmpfilename = _mktemp(tmppath2);
  if (!tmpfilename) {
    printf("could not allocate temporary file!\n");
	abort();
  }
#else
  char tmpfilename[] = "/tmp/openlfhdf5tempfileXXXXXX";
  int handle = mkstemp(tmpfilename);
  assert(handle != -1);
#endif
  
  //FIXME handle file delete at the end!
  H5File f = H5File(tmpfilename, H5F_ACC_TRUNC, FileCreatPropList::DEFAULT, acc_plist);
  cf = ClifFile(f, tmpfilename);
#ifdef CLIF_COMPILER_MSVC
  free(tmppath);
#else
  close(handle);
#endif
  
  return cf;
}

void h5_create_path_groups(H5::H5File &f, boost::filesystem::path path) 
{
  boost::filesystem::path part;
  
  for(auto it = path.begin(); it != path.end(); ++it) {
    part /= *it;
    if (!clif::h5_obj_exists(f, part)) {
      f.createGroup(part.generic_string().c_str());
    }
  }
}


BaseType PredType_to_native_BaseType(H5::PredType type)
{    
  switch (type.getClass()) {
    case H5T_STRING : return BaseType::STRING;
    case H5T_INTEGER : return BaseType::INT;
    case H5T_FLOAT: return BaseType::DOUBLE;
    default:
      abort();
  }
  
  printf("ERROR: unknown argument type!\n");
  abort();
}

BaseType hid_t_to_native_BaseType(hid_t type)
{
  switch (H5Tget_class(type)) {
    case H5T_STRING : return BaseType::STRING;
    case H5T_INTEGER :
      switch (H5Tget_size(type)) {
        case 1 : 
          if (H5Tget_sign(type) != H5T_SGN_NONE)
            abort();
          return BaseType::UINT8;
        case 2 :
          if (H5Tget_sign(type) != H5T_SGN_NONE)
            abort();
          return BaseType::UINT16;
        case 3 :
        case 4 :
          if (H5Tget_sign(type) == H5T_SGN_NONE)
            abort();
          return BaseType::INT;
      }
    case H5T_FLOAT: 
      if (H5Tget_size(type) == 4)
        return BaseType::FLOAT;
      else if (H5Tget_size(type) == 8)
        return BaseType::DOUBLE;
      break;
    default:
      printf("ERROR: unknown argument type!\n");
      abort();
  }
}

H5::PredType BaseType_to_PredType(BaseType type)
{
  switch (type) {
    case BaseType::STRING : return H5::PredType::C_S1;
    case BaseType::UINT8 : return H5::PredType::NATIVE_B8;
    case BaseType::UINT16 : return H5::PredType::NATIVE_B16;
    case BaseType::INT : return H5::PredType::NATIVE_INT;
    case BaseType::FLOAT: return H5::PredType::NATIVE_FLOAT;
    case BaseType::DOUBLE: return H5::PredType::NATIVE_DOUBLE;
    default:
    printf("ERROR: unknown argument type!\n");
      abort();
  }
}

  
}