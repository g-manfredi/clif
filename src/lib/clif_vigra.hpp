#ifndef _CLIF_VIGRA_H
#define _CLIF_VIGRA_H

#include "clif.hpp"
#include "subset3d.hpp"

#include <vigra/multi_array.hxx>
#include <vigra/multi_shape.hxx>

namespace clif {
  
/** \defgroup clif_vigra Vigra Bindings
 *  @{
 */
  
  //same as callByBaseType but without char (cause vigra doesn't like it :-))
  template<template<typename> class F, typename ... ArgTypes> void callByBaseType_Vigra(BaseType type, ArgTypes & ... args)
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
  
  template<template<typename> class F, typename R, typename ... ArgTypes> R callByBaseType_Vigra_r(BaseType type, ArgTypes & ... args)
  {
    switch (type) {
      case BaseType::UINT8 :  return F<uint8_t>()(args...); break;
      case BaseType::UINT16 : return F<uint16_t>()(args...); break;
      case BaseType::INT :    return F<int>()(args...); break;
      case BaseType::FLOAT :  return F<float>()(args...); break;
      case BaseType::DOUBLE : return F<double>()(args...); break;
      default:
        abort();
    }/** \defgroup clif_vigra Vigra Bindings
 *  @{
 */
  }
  
  template<uint DIM> class FlexMAV;
  template <uint DIM, typename T, uint IDX> vigra::MultiArrayView<DIM,T> getFixedChannel(FlexMAV<DIM> &a);
  
  template<uint DIM> class FlexMAV {
  public:
    
    typedef vigra::TinyVector<vigra::MultiArrayIndex, DIM> difference_type;
    
    FlexMAV() {};
    //FlexMAV(const difference_type &shape, BaseType type) : _shape(shape), _type(type) {}
    FlexMAV(const difference_type &shape, BaseType type, std::vector<cv::Mat> inputs) { create(shape,type,inputs); };
    
    template<typename T> class new_channels_dispatcher {
    public:
      void * operator()(FlexMAV<DIM> &mav, std::vector<cv::Mat> &inputs)
      {
        auto channels = new std::vector<vigra::MultiArrayView<DIM,T>>(inputs.size());
        for(int i=0;i<inputs.size();i++)
          (*channels)[i] = vigra::MultiArrayView<DIM,T>(mav.shape(), (T*)inputs[i].data);
        
        return channels;
      }
    };
    
    void create(difference_type shape, BaseType type, std::vector<cv::Mat> inputs)
    { 
      _shape = shape;
      _type = type;
      _mats = inputs;
      _ch_count = inputs.size();
      //_mems.resize(0);
      if (_channels)
        delete _channels;
      _channels = call_r<new_channels_dispatcher,void*>(*this, inputs);
    }
    
    template<template<typename> class F, typename ... ArgTypes> void call(ArgTypes & ... args) { callByBaseType_Vigra<F>(_type, args...); }
    template<template<typename> class F, typename R, typename ... ArgTypes> R call_r(ArgTypes & ... args) { return callByBaseType_Vigra_r<F,R>(_type, args...); }
    
    template<template<typename> class F, typename ... ArgTypes> void callChannels(ArgTypes & ... args)
    {
      //F<DIM>(args...);
      for(int i=0;i<_ch_count;i++)
        call<F>(i, args...);
    }
    /*template<template<typename> class F, typename R, typename ... ArgTypes> R callChannels(ArgTypes ... args) { return callMAVByBaseType<F,DIM>(_type, args...); }*/
    
    void add(void *channel);
    
    template<typename T> std::vector<vigra::MultiArrayView<DIM,T>> *channels() { return static_cast<std::vector<vigra::MultiArrayView<DIM,T>> *>(_channels); }
    template<typename T> vigra::MultiArrayView<DIM,T>& channel(int n) { return (*static_cast<std::vector<vigra::MultiArrayView<DIM,T>> *>(_channels))[n]; }
    
    difference_type shape() { return _shape; };
    
    //template<typename T, uint idx> vigra::MultiArrayView<DIM,T> &channel() { return channels<T>[idx]; }
    
  private:
    int _ch_count;
    difference_type _shape;
    std::vector<cv::Mat> _mats;
    //std::vector<void*> _mems;
    void *_channels = NULL;
    BaseType _type = BaseType::INVALID;
  };

  template <uint DIM, typename T, uint IDX> vigra::MultiArrayView<DIM,T>* getFixedChannel(FlexMAV<DIM> &a)
  {
    return a.channels()[IDX];
  }
    
  vigra::Shape2 imgShape(Datastore *store);
  
  void readImage(Datastore *store, uint idx, void **channels, int flags = 0, float scale = 1.0);
  void readImage(Datastore *store, uint idx, FlexMAV<2> &channels, int flags = 0, float scale = 1.0);
  
  void readEPI(Subset3d *subset, void **channels, int line, double disparity, ClifUnit unit = ClifUnit::PIXELS, int flags = 0, Interpolation interp = Interpolation::LINEAR, float scale = 1.0);
  
  void readEPI(Subset3d *subset, FlexMAV<2> &channels, int line, double disparity, ClifUnit unit = ClifUnit::PIXELS, int flags = 0, Interpolation interp = Interpolation::LINEAR, float scale = 1.0);
  
  //void readSubset3d(Datastore *store, uint idx, void **volume, int flags = 0, float scale = 1.0);
  
/**
 *  @}
 */
  
}

#endif