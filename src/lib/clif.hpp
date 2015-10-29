#ifndef _CLIF_H
#define _CLIF_H

#include <vector>
#include <iostream>

#include "core.hpp"
#include "attribute.hpp"
#include "stringtree.hpp"
#include "datastore.hpp"
#include "dataset.hpp"

#include "config.h"


namespace clif {
  
//std::string path_element(boost::filesystem::path path, int idx);
  
  /*int parse_string_enum(std::string &str, const char **enumstrs);
  int parse_string_enum(const char *str, const char **enumstrs);*/
  
#define DEMOSAIC  1
#define CVT_8U  2
#define UNDISTORT 4
#define CVT_GRAY 8
#define NO_DISK_CACHE 16
#define NO_MEM_CACHE 32
#define CACHE_FORCE_MEMCPY 64


#define PROCESS_FLAGS_MAX 128

H5::PredType H5PredType(BaseType type);
  
std::vector<std::string> Datasets(H5::H5File &f);

CLIF_EXPORT class ClifFile
{
public:
  ClifFile() {};
  //TODO create file if not existing?
  ClifFile(const std::string &filename, unsigned int flags = H5F_ACC_RDONLY);
  
  void open(const std::string &filename, unsigned int flags = H5F_ACC_RDONLY);
  void create(const std::string &filename);
  //void close();
  
  clif::Dataset* openDataset(int idx);
  clif::Dataset* openDataset(const std::string name = std::string());

  clif::Dataset* createDataset(const std::string name);
  
  int datasetCount();
  
  bool valid();
  
  const std::vector<std::string> & datasetList() const {return datasets;};
  
  H5::H5File f;
private:
  
  std::vector<std::string> datasets;
};

}

#endif