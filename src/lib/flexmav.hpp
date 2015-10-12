#ifndef _CLIF_FLEXMAV_H
#define _CLIF_FLEXMAV_H

#include <vigra/multi_array.hxx>
#include <vigra/multi_shape.hxx>
#include <vigra/imageinfo.hxx>
#include <vigra/impex.hxx>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "enumtypes.hpp"
#include "clif.hpp"
#include "hdf5.hpp"

namespace clif {
  
  //same as callByBaseType but without char (cause vigra doesn't like it :-))
  template<template<typename> class F, typename ... ArgTypes> void callByBaseType_flexmav(BaseType type, ArgTypes ... args)
  {
    switch (type) {
      case BaseType::UINT8 :  F<uint8_t>()(args...); break;
      case BaseType::UINT16 : F<uint16_t>()(args...); break;
      case BaseType::INT :    F<int>()(args...); break;
      case BaseType::FLOAT :  F<float>()(args...); break;
      case BaseType::DOUBLE : F<double>()(args...); break;
      default:
        abort();
    }
  }
  
  template<template<typename> class F, typename R, typename ... ArgTypes> R callByBaseType_flexmav_r(BaseType type, ArgTypes ... args)
  {
    switch (type) {
      case BaseType::UINT8 :  return F<uint8_t>()(args...); break;
      case BaseType::UINT16 : return F<uint16_t>()(args...); break;
      case BaseType::INT :    return F<int>()(args...); break;
      case BaseType::FLOAT :  return F<float>()(args...); break;
      case BaseType::DOUBLE : return F<double>()(args...); break;
      default:
        abort();
    }
  }

  static BaseType vigraPixelType2BaseType(vigra::ImageImportInfo::PixelType type){
    switch (type) {
      case vigra::ImageImportInfo::PixelType::UINT8 : return BaseType::UINT8; break;
      case vigra::ImageImportInfo::PixelType::UINT16 : return BaseType::UINT16; break;
      case vigra::ImageImportInfo::PixelType::INT32 : return BaseType::INT; break;
      case vigra::ImageImportInfo::PixelType::FLOAT : return BaseType::FLOAT; break;
      case vigra::ImageImportInfo::PixelType::DOUBLE : return BaseType::DOUBLE; break;
      default:
        return BaseType::INVALID;
    }
  }
  
  template<uint DIM> class FlexMAV;
  template <uint DIM, typename T, uint IDX> vigra::MultiArrayView<DIM,T> getFixedChannel(FlexMAV<DIM> &a);
  
  template<uint DIM> class FlexMAV {
  public:
    
    typedef vigra::TinyVector<vigra::MultiArrayIndex, DIM> difference_type;
    
    FlexMAV() {};
    FlexMAV(const difference_type &shape, BaseType type, std::vector<cv::Mat> inputs) { create(shape,type,inputs); };
    FlexMAV(const difference_type &shape, BaseType type) { create(shape,type); };
    
    template<typename T> class new_dispatcher {
    public:
      void * operator()(FlexMAV<DIM> *mav, cv::Mat *input)
      {
        return new vigra::MultiArrayView<DIM,T>(mav->shape(), (T*)input->data);
      }
    };
    
    template<typename T> class delete_dispatcher {
    public:
      void operator()(FlexMAV<DIM> *mav)
      {
        vigra::MultiArrayView<DIM,T> *img = mav->template get<T>();
        delete img;
      }
    };

    template<typename T> class importImage_dispatcher {
    public:
        void operator()(FlexMAV<DIM> &mav, std::string name)
        {
          vigra::MultiArrayView<DIM,T> *img = mav.template get<T>();
          vigra::importImage(name,mav);
        }
    };

    template<typename T> class exportImage_dispatcher {
    public:
        void operator()(FlexMAV<DIM> *mav, const char *name)
        {
          vigra::MultiArrayView<DIM,T> *img = mav->get<T>();
          vigra::exportImage(*img,name);
        }
    };
    
    void create(difference_type shape, BaseType type, cv::Mat &input)
    {
      if (_data)
        call<delete_dispatcher>(this);
      _shape = shape;
      _type = type;
      _mat = input;
      _data = call_r<new_dispatcher,void*>(this, &_mat);
    }
    
    //read from datastore
    void read(Datastore *store)
    {
      if (_data)
        call<delete_dispatcher>(this);
      
      store->read(_mat);
      assert(_mat.dims == DIM);
      
      _type = CvDepth2BaseType(_mat.depth());
    
      for (int i=0;i<DIM;i++)
        _shape[i] = _mat.size[DIM-i-1];
      
      _data = call_r<new_dispatcher,void*>(this, &_mat);
    }
    
    //write to datastore
    void write(Datastore *store)
    {
      assert(_data);
      
      store->write(_mat);
    }
    
    //TODO delay actual creation until first needed (may save malloc)
    void create(difference_type shape, BaseType type)
    {
      if(_type == type || _shape == shape)
        return;

      if (_data)
        call<delete_dispatcher>(this);
      
      int size[DIM];
      for (int i = 0; i < DIM; i++)
        size[i] = shape[DIM-i-1];
      
      _type = type;
      _shape = shape;
      
      _mat = cv::Mat(DIM, size, BaseType2CvDepth(_type));
      _data = call_r<new_dispatcher, void *>(this, &_mat);
    }
    
    void reshape(difference_type shape)
    { 
      create(shape, _type);
    }

    void importImage(std::string filename)
    {
      vigra::ImageImportInfo info(filename.c_str());

      //Getting image shape
      vigra::Shape2 shape(info.width() , info.height() );

      //Getting data type and converting to BaseType
      BaseType type = vigraPixelType2BaseType(info.pixelType());

      create(shape,type);
      call<importImage_dispatcher>(this, filename);
    }

    void exportImage(const char *filename)
    {
      call<exportImage_dispatcher>(this, filename);
    }
    
    void write(Dataset *set, path path)
    {
      boost::filesystem::path fullpath = set->path() / path;
      
      if (h5_obj_exists(set->f, fullpath)) {
        printf("TODO overwrite!\n");
        abort();
      }
      
      h5_create_path_groups(set->f, fullpath.parent_path());
      
      hsize_t dims[DIM];
      for(int i=0;i<DIM;i++)
        dims[i] = _shape[DIM-i-1];
      
      //H5::DSetCreatPropList prop;    
      //prop.setChunk(dimcount, chunk_dims);
      
      H5::DataSpace space(DIM, dims, dims);
      
      H5::DataSet h5set = set->f.createDataSet(fullpath.c_str(), 
                          H5PredType(_type), space);
      
      h5set.write(_mat.data, H5::DataType(H5PredType_Native(_type)), space, space);
      /*
      if (DIM == 2)
        cv::imwrite("debug_cv.tiff", _mat);
      else
        printf("dim: %d\n", DIM);
      
      exportImage("debug_vigra.tiff");*/
    }

    
    template<template<typename> class F, typename ... ArgTypes> void call(ArgTypes ... args) { callByBaseType_flexmav<F>(_type, args...); }
    template<template<typename> class F, typename R, typename ... ArgTypes> R call_r(ArgTypes ... args) { return callByBaseType_flexmav_r<F,R>(_type, args...); }
    
    
    template<typename T> vigra::MultiArrayView<DIM,T> *get() { return static_cast<vigra::MultiArrayView<DIM,T>*>(_data); }
    difference_type shape() { return _shape; };
    
    BaseType type() { return _type; }
    
  private:
    BaseType _type = BaseType::INVALID;
    difference_type _shape;
    cv::Mat _mat;
    void *_data = NULL;
  };
}

#endif