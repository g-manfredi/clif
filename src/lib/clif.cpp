#include "clif.hpp"

#include "cliini.h"

#include <string>
#include <assert.h>

#include <exception>

#include "dataset.hpp"
#include "hdf5.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <sys/types.h>
#include <sys/stat.h>

namespace clif {

typedef unsigned int uint;
  
namespace {
  hid_t _fapl_create()
  {
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    //should be H5Pset_libver_bounds(fapl, H5F_LIBVER_18, H5F_LIBVER_LATEST); but it seems H5F_LIBVER_18 is gone with 1.10!
    H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
    
    return fapl;
  }

  static hid_t _file_acc_plist = _fapl_create();
}

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
  _path = get_abs_path(filename);
  
  try {
    f.openFile(filename.c_str(), flags, H5::FileAccPropList(_file_acc_plist));
  }
  catch (H5::FileIException e) {
    printf("opening %s failed!\n", filename.c_str());
    assert(f.getId() == H5I_INVALID_HID);
    return;
  }
  
  datasets.resize(0);
  
  if (f.getId() == H5I_INVALID_HID) {
    printf("error opening file!\n");
    return;
  }
  
  if (!h5_obj_exists(f, "/clif"))
      return;
    
  H5::Group g = f.openGroup("/clif");
  
  hsize_t count = g.getNumObjs();
  datasets.resize(count);
  
  for (uint i = 0; i < count; i++) {
    char name[1024];
    g.getObjnameByIdx(i, name, 1024);
    datasets[i] = std::string(name);
  }
}

//FIXME delete datasets?
void ClifFile::close()
{
  flush();

  f.close();
  
  datasets.resize(0);
}

void ClifFile::flush()
{
  f.flush(H5F_SCOPE_LOCAL);
}

void ClifFile::create(const std::string &filename)
{
  _path = boost::filesystem::absolute(filename);
  
  f = H5::H5File(filename.c_str(), H5F_ACC_TRUNC, H5::FileCreatPropList::DEFAULT, H5::FileAccPropList(_file_acc_plist));
  
  datasets.resize(0);
  
  if (f.getId() == H5I_INVALID_HID) {
    printf("could not open %s\n", filename.c_str());
    return;
  }
  
  //FIXME there will enver be /clif in a truncated file!
  if (!h5_obj_exists(f, "/clif"))
      return;
//     
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

ClifFile::ClifFile(H5::H5File h5file, boost::filesystem::path &&path)
{
  //FIXME set libver?
  f = h5file;
  _path = path;
}

int ClifFile::datasetCount()
{
  return datasets.size();
}


Dataset* ClifFile::openDataset(const std::string name)
{
  Dataset *set = new Dataset();
  
  if (name.size())
    set->open(*this, name);
  else
    set->open(*this, datasets[0]);

  return set;
}

Dataset* ClifFile::createDataset(const std::string name)
{
  Dataset *set = new Dataset();
  set->create(*this, name);
  datasets.push_back(name);
  return set;
}

time_t ClifFile::mtime()
{
  struct stat st;
  stat(_path.string().c_str(), &st);
  
  return st.st_mtime;
}

clif::Dataset* ClifFile::openDataset(int idx)
{
  return openDataset(datasets[idx]);
}

bool ClifFile::valid()
{
  //FIXME check hdf5 version?
  //old (at least on ubunut 14.04 - 1.8.11-5ubuntu7) hdf5 versions don't init H5File id to H5I_INVALID_HID (== -1) but to zero...)
  if (f.getId() <= 0)
    return false;

  return f.getId() != H5I_INVALID_HID;
}

const cpath& ClifFile::path()
{
  return _path;
}

}
