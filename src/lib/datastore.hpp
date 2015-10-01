#ifndef _CLIF_DATASTORE_H
#define _CLIF_DATASTORE_H

#include "opencv2/core/core.hpp"

#include "core.hpp"

namespace clif {
class Dataset;

//representation of a "raw" clif datset - mostly the images
class Datastore {
  public:
	  Datastore() { _imgsize[0] = -1; _imgsize[1] = -1; };
    
    //create new datastore
    void create(std::string path, Dataset *dataset);
    
    //create new datastore with specified size and type
    //void create(std::string path, Dataset *dataset, BaseType type, int dims, int *size);
    //create from opencv matrix
    void create(std::string path, Dataset *dataset, cv::Mat &m);
    
    //create this datastore as a link to other in dataset - dataset is then readonly!
    void link(const Datastore *other, Dataset *dataset);
    
    //open existing datastore
    void open(Dataset *dataset, std::string path);
    
    void writeRawImage(uint idx, hsize_t w, hsize_t h, void *data);
    void appendRawImage(hsize_t w, hsize_t h, void *data);
    void readRawImage(uint idx, hsize_t w, hsize_t h, void *data);
    
    //read store into m 
    void read(cv::Mat &m);
    //write m into store
    void write(cv::Mat &m);
    
    int imgMemSize();
    
    bool valid();
    void size(int s[3]);
    void imgSize(int s[2]);
    int channels() { return 3; } //FIXME grayscale!?
    int count();
    
    const std::string& getDatastorePath() const { return _path; };
    
    const H5::DataSet & H5DataSet() const { return _data; };
    Dataset *getDataset() { return _dataset; };
    const BaseType & type() const { return _type; };
    const DataOrg & org() const { return _org; };
    const DataOrder & order() const { return _order; };
    
    //FIXME delete cache contents on desctructor
    
    void *cache_get(int idx, int flags, float scale);
    void cache_set(int idx, int flags, float scale, void *data);

    template<template<typename> class F, typename R, typename ... ArgTypes> R call(ArgTypes ... args)
    {
      return callByBaseType<F>(_type, args...);
    }
    
    template<template<typename> class F, typename ... ArgTypes> void call(ArgTypes ... args)
    {
      callByBaseType<F>(_type, args...);
    }
    
  protected:
    void init(hsize_t w, hsize_t h);
    
    BaseType _type; 
    DataOrg _org;
    DataOrder _order;
    
    H5::DataSet _data;
    std::string _path;
    
private:
  std::unordered_map<uint64_t,void*> image_cache;
  
  Dataset *_dataset = NULL;
  bool _readonly = false; //linked dataset - convert to not-linked data to make read/write (no doing this implicitly may save a copy)
  bool _memonly = false; //dataset may be in memory and linked or not linked - TODO check all possible cases and uses
  std::string _link_file;
  std::string _link_path;
  
  cv::Mat _mat;
  
  int _imgsize[2];
};

}

#endif