#include "clif.hpp"

#include <string>
#include <assert.h>

#include <exception>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "cliini.h"

namespace clif {
  
  
  /*std::string path_element(boost::filesystem::path path, int idx)
  {    
    auto it = path.begin();
    for(int i=0;i<idx;i++,++it) {
      if (it == path.end())
        throw std::invalid_argument("requested path element too large");
      if (!(*it).generic_string().compare("/"))
        ++it;
    }
    
    return (*it).generic_string();
  }*/  

  /*int parse_string_enum(const char *str, const char **enumstrs)
  {
    int i=0;
    while (enumstrs[i]) {
      if (!strcmp(str,enumstrs[i]))
        return i;
      i++;
    }
    
    return -1;
  }
  
  int parse_string_enum(std::string &str, const char **enumstrs)
  {
    int i=0;
    while (enumstrs[i]) {
      if (!str.compare(enumstrs[i]))
        return i;
      i++;
    }
    
    return -1;
  }*/
  

  
  
  
  
  
  /*void readOpenCvMat(uint idx, Mat &m);
        assert(w == img.size().width);
      assert(h = img.size().height);
      assert(depth = img.depth());
      printf("store idx %d: %s\n", i, in_names[i]);
      lfdata.writeRawImage(i, img.data);*/
  
  /*int Datastore::imgMemSize()
  {
    int size = 1;
    switch (type) {
      case DataType::UINT8 : break;
      case DataType::UINT16 : size *= 2; break;
    }
    switch (org) {
      case DataOrg::BAYER_2x2 : size *= 4; break;
    }
    
    H5::DataSpace space = data.getSpace();
    hsize_t dims[3];
    hsize_t maxdims[3];
    
    space.getSimpleExtentDims(dims, maxdims);
    
    return size*dims[0]*dims[1];
  }*/
  
  std::vector<std::string> Datasets(H5::H5File &f)
  {
    std::vector<std::string> list(0);
    
    if (!h5_obj_exists(f, "/clif"))
      return list;
    
    H5::Group g = f.openGroup("/clif");
    
    hsize_t count = g.getNumObjs();
    list.resize(count);
    
    for(uint i=0;i<count;i++)
      list[i] = g.getObjnameByIdx(i);
    
    return list;
  }


void ClifFile::open(const std::string &filename, unsigned int flags)
{
  f.openFile(filename, flags);
  
  datasets.resize(0);
  
  if (f.getId() == H5I_INVALID_HID)
    return;
  
  if (!clif::h5_obj_exists(f, "/clif"))
      return;
    
  H5::Group g = f.openGroup("/clif");
  
  hsize_t count = g.getNumObjs();
  datasets.resize(count);
  
  for(uint i=0;i<count;i++)
    datasets[i] = g.getObjnameByIdx(i);
}

void ClifFile::create(const std::string &filename)
{
  f = H5::H5File(filename, H5F_ACC_TRUNC);
  
  datasets.resize(0);
  
  if (f.getId() == H5I_INVALID_HID)
    return;
  
  if (!clif::h5_obj_exists(f, "/clif"))
      return;
    
  H5::Group g = f.openGroup("/clif");
  
  hsize_t count = g.getNumObjs();
  datasets.resize(count);
  
  for(uint i=0;i<count;i++)
    datasets[i] = g.getObjnameByIdx(i);
}

ClifFile::ClifFile(const std::string &filename, unsigned int flags)
{
  open(filename, flags);
}

int ClifFile::datasetCount()
{
  return datasets.size();
}


clif::Dataset* ClifFile::openDataset(const std::string name)
{
  clif::Dataset *set = new clif::Dataset();
  
  if (name.size())
    set->open(f, name);
  else
    set->open(f, datasets[0]);

  return set;
}

clif::Dataset* ClifFile::createDataset(const std::string name)
{
  clif::Dataset *set = new clif::Dataset();
  set->create(f, name);
  return set;
}

clif::Dataset* ClifFile::openDataset(int idx)
{
  return openDataset(datasets[idx]);
}

bool ClifFile::valid()
{
  return f.getId() != H5I_INVALID_HID;
}

}
