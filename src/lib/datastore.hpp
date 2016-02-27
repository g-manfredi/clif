#ifndef _CLIF_DATASTORE_H
#define _CLIF_DATASTORE_H

#include "mat.hpp"
#include "opencv2/core/core.hpp"

#include "core.hpp"
#include "preproc.hpp"


namespace clif {
    
  class Read_Options {
  public:
    Read_Options(const Idx &idx_,
                 int flags_ = 0,
                    double depth_ = std::numeric_limits<double>::quiet_NaN(),
                    double min_ = std::numeric_limits<double>::quiet_NaN(),
                    double max_ = std::numeric_limits<double>::quiet_NaN(),
                    int scale_ = 0,
                    int extra_flags_ = 0
                   )
    : flags(flags_), depth(depth_), min(min_), max(max_), scale(scale_), extra_flags(extra_flags_)
    {
      idx = idx_;
      
    }
    
    inline bool operator== (const Read_Options &other) const
    {      
      bool res = ((idx == other.idx) && 
             (flags == other.flags) &&
             (min == other.min || (std::isnan(min) && std::isnan(other.min))) &&
             (max == other.max || (std::isnan(max) && std::isnan(other.max))) &&
             (depth == other.depth) &&
             (scale == other.scale) &&
             (extra_flags == other.extra_flags));
      
      return res;
    }
    
    Idx idx;
    int flags;
    double min, max;
    double depth;
    int scale;
    int extra_flags;
  };

}

namespace std
{
    template<>
    struct hash<clif::Read_Options>
    { 
        std::size_t operator()(clif::Read_Options const& opts) const
        {
          std::size_t h = std::hash<int>()(opts.flags);
          h ^= std::hash<double>()(opts.min) << 1;
          h ^= std::hash<double>()(opts.max) << 2;
          h ^= std::hash<double>()(opts.depth) << 3;
          h ^= std::hash<int>()(opts.scale) << 4;
          h ^= std::hash<int>()(opts.extra_flags) << 5;
          h ^= std::hash<int>()(opts.idx.size()) << 6;
          for(int i=0;i<opts.idx.size();i++)
            h ^= std::hash<int>()(opts.idx[i]) << (7+i);
          
          return h;
        }
    };
}

namespace clif {
class Dataset;
class DepthDist;

//representation of a "raw" clif datset - mostly the images
class Datastore {
public:
    //needed for Dataset to call the protected constructure - this should not be necessary!
    //TODO maybe the problem is that dataset inherits from datastore as second part of multi-inheritance?
    friend class Dataset;
    
    //create new datastore
    void create(const cpath & path, Dataset *dataset);
    
    //create new datastore with specified size and type
    //void create(std::string path, Dataset *dataset, BaseType type, int dims, int *size);
    //create from opencv matrix
    void create(const cpath & path, Dataset *dataset, cv::Mat &m);
    
    //create this datastore as a link to other in dataset - dataset is then readonly!
     void link(const Datastore *other, Dataset *dataset);
    
    //open existing datastore
    void open(Dataset *dataset, cpath path);
    
    void writeRawImage(int idx, hsize_t w, hsize_t h, void *data);
    void appendRawImage(hsize_t w, hsize_t h, void *data);
    void readRawImage(int idx, hsize_t w, hsize_t h, void *data);
    
    void writeChannel(const std::vector<int> &idx, cv::Mat *img);
    void writeImage(const std::vector<int> &idx, cv::Mat *img);
    void appendImage(cv::Mat *img);
    
    void readImage(const Idx& idx, cv::Mat *img, int flags, double depth = std::numeric_limits<float>::quiet_NaN(), double min = std::numeric_limits<float>::quiet_NaN(), double max = std::numeric_limits<float>::quiet_NaN());
    void readImage(const Idx& idx, cv::Mat *img, const ProcData &proc = ProcData());
    void readChannel(const Idx& idx, cv::Mat *channel, int flags = 0);
    
    void setDims(int dims);
    
    //read store into m 
    void read(cv::Mat &m);
    void read(clif::Mat &m);
    void read(clif::Mat &m, const ProcData &proc);
    void read_full_subdims(Mat &m, std::vector<int> dim_order, Idx offset);
    //write m into store
    void write(cv::Mat &m);
    void write(const clif::Mat &m);
    void write(const clif::Mat * const m);
    void write(const Mat &m, const Idx &pos);
    void append(const Mat &m);
    
    int imgMemSize();
    
    bool valid() const;
    int dims() const;
    const std::vector<int>& extent() const;
    void fullsize(std::vector<int> &size) const;
    
    int imgCount();
    
    void flush();
    void reset();
    
    const cpath& path() const { return _path; };
    cpath fullPath() const;
    
    //actual file path (might be from linked file!)
    cpath io_path();
    time_t mtime();
    
    friend std::ostream& operator<<(std::ostream& out, const Datastore& a);
    
    const H5::DataSet & H5DataSet() const { return _data; };
    Dataset *dataset() { return _dataset; };
    const BaseType & type() const { return _type; };
    const DataOrg & org() const { return _org; };
    const DataOrder & order() const { return _order; };
    
    DepthDist* undist(double depth);
    
    //FIXME delete cache contents on desctructor
    
    void *cache_get(const Read_Options &opts);
    void cache_set(const Read_Options &opts, void *data);
    void cache_flush();
    bool mat_cache_get(clif::Mat *m, const Read_Options &opts);
    void mat_cache_set(const Read_Options &opts, clif::Mat *m);

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
    void init_write(int dims, const Idx &img_size, BaseType type);
    void create_types(BaseType type = BaseType::INVALID);
    void create_dims_imgs(const Idx& size);
    void create_store();
    
    BaseType _type = BaseType::INVALID;
    DataOrg _org = DataOrg::INVALID;
    DataOrder _order = DataOrder::INVALID;
    
    H5::DataSet _data;
    cpath _path;
    
private:
  std::unordered_map<Read_Options,void*> image_cache;
  
  Dataset *_dataset = NULL;
  bool _readonly = false; //linked dataset - convert to not-linked data to make read/write (no doing this implicitly may save a copy)
  bool _memonly = false; //dataset may be in memory and linked or not linked - TODO check all possible cases and uses
  cpath _link_file;
  cpath _link_path;
  
  bool _undist_loaded = false;
  //FIXME which undist for depth dependend...
  //the implemented tree-derive model is not very good (need to search subGroup each time!)
  //use softlink!?
  std::unordered_map<double,DepthDist*> _undists;
  
  clif::Mat _mat;
  
  Dataset& operator=(const Dataset& other) = delete;
  
  //size of img (w chanels, others 0)
  std::vector<int> _basesize;
  //actual dataset size
  std::vector<int> _extent;
};

}

#endif
