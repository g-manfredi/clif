#ifndef _CLIF_DATASTORE_H
#define _CLIF_DATASTORE_H

#include "opencv2/core/core.hpp"

#include "core.hpp"
#include "mat.hpp"

namespace clif {
class Dataset;

//representation of a "raw" clif datset - mostly the images
class Datastore {
public:
    //needed for Dataset to call the protected constructure - this should not be necessary!
    //TODO maybe the problem is that dataset inherits from datastore as second part of multi-inheritance?
    friend class Dataset;
    
    //create new datastore
    void create(std::string path, Dataset *dataset, const std::string format_group = std::string());
    
    //create new datastore with specified size and type
    //void create(std::string path, Dataset *dataset, BaseType type, int dims, int *size);
    //create from opencv matrix
    void create(std::string path, Dataset *dataset, cv::Mat &m, const std::string format_group = std::string());
    
    //create this datastore as a link to other in dataset - dataset is then readonly!
     void link(const Datastore *other, Dataset *dataset);
    
    //open existing datastore
    void open(Dataset *dataset, std::string path, const std::string format_group = std::string());
    
    void writeRawImage(int idx, hsize_t w, hsize_t h, void *data);
    void appendRawImage(hsize_t w, hsize_t h, void *data);
    void readRawImage(int idx, hsize_t w, hsize_t h, void *data);
    
    void writeChannel(const std::vector<int> &idx, cv::Mat *img);
    void writeImage(const std::vector<int> &idx, cv::Mat *img);
    void appendImage(cv::Mat *img);
    
    void readImage(const std::vector<int> &idx, cv::Mat *img, int flags = 0, double min = std::numeric_limits<float>::quiet_NaN(), double max = std::numeric_limits<float>::quiet_NaN());
    void readChannel(const std::vector<int> &idx, cv::Mat *channel, int flags = 0);
    
    void setDims(int dims);
    
    //read store into m 
    void read(cv::Mat &m);
    void read(clif::Mat &m);
    //write m into store
    void write(cv::Mat &m);
    void write(clif::Mat &m);
    void write(clif::Mat *m);
    
    int imgMemSize();
    
    bool valid() const;
    int dims() const;
    const std::vector<int>& extent() const;
    void fullsize(std::vector<int> &size) const;
    
    int imgChannels();
    int imgCount();
    
    void flush();
    void reset();
    
    const std::string& getDatastorePath() const { return _path; };
    
    friend std::ostream& operator<<(std::ostream& out, const Datastore& a);
    
    const H5::DataSet & H5DataSet() const { return _data; };
    Dataset *getDataset() { return _dataset; };
    const BaseType & type() const { return _type; };
    const DataOrg & org() const { return _org; };
    const DataOrder & order() const { return _order; };
    
    //FIXME delete cache contents on desctructor
    
    void *cache_get(const std::vector<int> idx, int flags, int extra_flags, float scale);
    void cache_set(const std::vector<int> idx, int flags, int extra_flags, float scale, void *data);
    bool mat_cache_get(cv::Mat *m, const std::vector<int> idx, int flags, int extra_flags, float scale);
    void mat_cache_set(cv::Mat *m, const std::vector<int> idx, int flags, int extra_flags, float scale);

    template<template<typename> class F, typename R, typename ... ArgTypes> R call(ArgTypes ... args)
    {
      return callByBaseType<F>(_type, args...);
    }
    
    template<template<typename> class F, typename ... ArgTypes> void call(ArgTypes ... args)
    {
      callByBaseType<F>(_type, args...);
    }
    
  protected:
    //datastore is tied to dataset!
    Datastore() {};
    ~Datastore();
    
    //initialization
    void init_write(const std::vector<int> &idx, cv::Mat *img);
    void create_types(BaseType type = BaseType::INVALID);
    void create_dims_imgs(int w, int h, int chs);
    void create_store();
    
    BaseType _type = BaseType::INVALID;
    DataOrg _org = DataOrg::INVALID;
    DataOrder _order = DataOrder::INVALID;
    
    H5::DataSet _data;
    std::string _path;
    
private:
  std::unordered_map<uint64_t,void*> image_cache;
  
  Dataset *_dataset = NULL;
  bool _readonly = false; //linked dataset - convert to not-linked data to make read/write (no doing this implicitly may save a copy)
  bool _memonly = false; //dataset may be in memory and linked or not linked - TODO check all possible cases and uses
  std::string _link_file;
  std::string _link_path;
  
  clif::Mat _mat;
  
  Dataset& operator=(const Dataset& other) = delete;
  
  std::string _format_group;
  
  //size of img (w chanels, others 0)
  std::vector<int> _basesize;
  //actual dataset size
  std::vector<int> _extent;
};

}

#endif