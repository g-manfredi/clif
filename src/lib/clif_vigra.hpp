#ifndef _CLIF_VIGRA_H
#define _CLIF_VIGRA_H

#include "clif.hpp"
#include "subset3d.hpp"

#include <vigra/multi_array.hxx>
#include <vigra/multi_shape.hxx>

namespace clif {
  
  template<uint DIM> class FlexMAV;
  template <uint DIM, typename T, uint IDX> vigra::MultiArrayView<DIM,T> getFixedChannel(FlexMAV<DIM> &a);
  
  //fixed dim of 2 for example
  template<typename T, typename CHANNEL> void processsomethingperchannel(FlexMAV<2> &a, FlexMAV<2> &b)
  {
    vigra::MultiArrayView<2,T> *ch = CHANNEL(b);
    
    a *= 2;
  }

  template<typename CH, uint DIM, uint IDX, template <typename, CH (*channel_function)(FlexMAV<DIM>&)> typename F, typename T, typename ...ARGTS> void call_channel_MAV_type(ARGTS...args)
  {
    F<T,getFixedChannel<DIM,T,IDX>>(args...);
  }
  
  template<uint DIM, uint IDX, template <typename, void* (*channel_function)(FlexMAV<DIM>&)> typename F, typename T, typename ...ARGTS> void call_channel_type(ARGTS...args)
  {
    F<T,getFixedChannel<DIM,T,IDX>>(args...);
  }
  
  template<uint DIM, template <typename, void* (*channel_function)(FlexMAV<DIM>&)> typename F, typename ...ARGTS> void call_channel(BaseType type, uint idx, ARGTS ... args)
  {
    printf("call channel!\n");
    switch (type) {
      case BaseType::UINT8 :  call_channel_MAV_type<vigra::MultiArrayView<DIM,uint8_t>,DIM,idx,F,uint8_t>(args...); break;
      case BaseType::UINT16 : call_channel_MAV_type<vigra::MultiArrayView<DIM,uint16_t>,DIM,idx,F,uint16_t>(args...); break;
      case BaseType::INT :    call_channel_MAV_type<vigra::MultiArrayView<DIM,int>,DIM,idx,F,int>(args...); break;
      case BaseType::FLOAT :  call_channel_MAV_type<vigra::MultiArrayView<DIM,float>,DIM,idx,F,float>(args...); break;
      case BaseType::DOUBLE : call_channel_MAV_type<vigra::MultiArrayView<DIM,double>,DIM,idx,F,double>(args...); break;
      case BaseType::STRING : call_channel_MAV_type<vigra::MultiArrayView<DIM,char>,DIM,idx,F,char>(args...); break;
      default:
        abort();
    }
  }
  /*
  //deep dark black c++ magic :-D
  template<template<typename M, template<typename M> (M*)(FlexMAV &a) ()> class F, uint DIM, uint IDX, typename ... ArgTypes> void callMAVByBaseType(BaseType type, ArgTypes ... args)
  {
    switch (type) {
      case BaseType::UINT8 :  F<vigra::MultiArrayView<DIM,uint8_t>, getFixedChannel<>>()(args...); break;
      case BaseType::UINT16 : F<vigra::MultiArrayView<DIM,uint16_t>>()(args...); break;
      case BaseType::INT :    F<vigra::MultiArrayView<DIM,int>>()(args...); break;
      case BaseType::FLOAT :  F<vigra::MultiArrayView<DIM,float>>()(args...); break;
      case BaseType::DOUBLE : F<vigra::MultiArrayView<DIM,double>>()(args...); break;
      case BaseType::STRING : F<vigra::MultiArrayView<DIM,char>>()(args...); break;
      default:
        abort();
    }
  }
  
    //deep dark black c++ magic :-D
  template<template<typename, class CHANNEL> class F, uint DIM, uint IDX, typename ... ArgTypes> void callMAVByBaseType(BaseType type, ArgTypes ... args)
  {
    switch (type) {
      case BaseType::UINT8 :  F<vigra::MultiArrayView<DIM,uint8_t>, getFixedChannel<>>()(args...); break;
      case BaseType::UINT16 : F<vigra::MultiArrayView<DIM,uint16_t>>()(args...); break;
      case BaseType::INT :    F<vigra::MultiArrayView<DIM,int>>()(args...); break;
      case BaseType::FLOAT :  F<vigra::MultiArrayView<DIM,float>>()(args...); break;
      case BaseType::DOUBLE : F<vigra::MultiArrayView<DIM,double>>()(args...); break;
      case BaseType::STRING : F<vigra::MultiArrayView<DIM,char>>()(args...); break;
      default:
        abort();
    }
  }*/

  template<template<typename> class F, uint DIM, uint IDX, typename R, typename ... ArgTypes> R callMAVByBaseType(BaseType type, ArgTypes ... args)
  {
    switch (type) {
      case BaseType::UINT8 :  return F<vigra::MultiArrayView<DIM,uint8_t>>()(args...); break;
      case BaseType::UINT16 : return F<vigra::MultiArrayView<DIM,uint16_t>>()(args...); break;
      case BaseType::INT :    return F<vigra::MultiArrayView<DIM,int>>()(args...); break;
      case BaseType::FLOAT :  return F<vigra::MultiArrayView<DIM,float>>()(args...); break;
      case BaseType::DOUBLE : return F<vigra::MultiArrayView<DIM,double>>()(args...); break;
      case BaseType::STRING : return F<vigra::MultiArrayView<DIM,char>>()(args...); break;
      default:
        abort();
    }
  }
  
  
  template<uint DIM> class FlexMAV {
  public:
    
    typedef vigra::TinyVector<vigra::MultiArrayIndex, DIM> difference_type;
    
    FlexMAV() {};
    //FlexMAV(const difference_type &shape, BaseType type) : _shape(shape), _type(type) {}
    FlexMAV(const difference_type &shape, BaseType type, std::vector<cv::Mat> inputs) { create(shape,type,inputs); };
    
    friend class construct_channels_dispatcher;
    
    template<typename T> class construct_channels_dispatcher {
    public:
      void operator()(FlexMAV<DIM> &mav, std::vector<cv::Mat> &inputs)
      {
        auto channels = mav.template channels<T>();
        channels = new std::vector<vigra::MultiArrayView<DIM,T>>(inputs.size());
        for(int i=0;i<inputs.size();i++)
          (*channels)[i] = vigra::MultiArrayView<DIM,T>(mav.shape(), (T*)inputs[i].data);
      }
    };
    
    template<typename T> void construct_channels(FlexMAV<DIM> &mav, std::vector<cv::Mat> &inputs)
    {
      auto channels = mav.template channels<T>();
      channels = new std::vector<vigra::MultiArrayView<DIM,T>>(inputs.size());
      for(int i=0;i<inputs.size();i++)
        (*channels)[i] = vigra::MultiArrayView<DIM,T>(mav.shape(), (T*)inputs[i].data);
    }
    
    void create(difference_type shape, BaseType type, std::vector<cv::Mat> inputs)
    { 
      _shape = shape;
      _type = type;
      _mats = inputs;
      _ch_count = inputs.size();
      //_mems.resize(0);
      if (_channels)
        delete _channels;
      //call<construct_channels_dispatcher>(*this, inputs);
      call<construct_channels()>(*this, inputs);
    }
    
    template<template<typename> class F, typename ... ArgTypes> void call(ArgTypes ... args) { callByBaseType<F>(_type, args...); }
    
    template<template<typename> class F, typename ... ArgTypes> void callF(ArgTypes ... args) { callByBaseType<F>(_type, args...); }
    
    template<template<typename> class F, typename R, typename ... ArgTypes> R call(ArgTypes ... args) { return callByBaseType<F>(_type, args...); }
    
    template<template<typename,typename> class F, typename ... ArgTypes> void callChannels(ArgTypes ... args)
    {
      //F<DIM>(args...);
      for(int i=0;i<_ch_count;i++)
        call_channel<DIM,F>(_type, i, args...);
    }
    /*template<template<typename> class F, typename R, typename ... ArgTypes> R callChannels(ArgTypes ... args) { return callMAVByBaseType<F,DIM>(_type, args...); }*/
    
    void add(void *channel);
    
    template<typename T> std::vector<vigra::MultiArrayView<DIM,T>> *channels() { return static_cast<std::vector<vigra::MultiArrayView<DIM,T>> *>(_channels); }
    
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
  
  //void readSubset3d(Datastore *store, uint idx, void **volume, int flags = 0, float scale = 1.0);
  
}

#endif