#include "hdf5.hpp"

#include <H5File.h>
#include <H5FaccProp.h>
#include <H5FcreatProp.h>

using namespace H5;

namespace clif {

bool h5_obj_exists(H5::H5File &f, const char * const path)
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

bool h5_obj_exists(H5::H5File &f, const std::string path)
{
  return h5_obj_exists(f,path.c_str());
}

bool h5_obj_exists(H5::H5File &f, const boost::filesystem::path path)
{
  return h5_obj_exists(f, path.c_str());
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
  
H5::H5File h5_memory_file()
{
  FileAccPropList acc_plist;
  acc_plist.setCore(16*1024, false);
  return H5File("/tmp/hdf5placeholder", H5F_ACC_TRUNC, FileCreatPropList::DEFAULT, acc_plist);
}

void h5_create_path_groups(H5::H5File &f, boost::filesystem::path path) 
{
  boost::filesystem::path part;
  
  for(auto it = path.begin(); it != path.end(); ++it) {
    part /= *it;
    if (!clif::h5_obj_exists(f, part)) {
      f.createGroup(part.string());
    }
  }
}
  
}