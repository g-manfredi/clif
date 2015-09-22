#ifndef _CLIF_FLEXMAV_H
#define _CLIF_FLEXMAV_H

#include <vigra/multi_array.hxx>
#include <vigra/multi_shape.hxx>

#include <opencv2/core/core.hpp>

#include "enumtypes.hpp"

namespace clif {
  
  //same as callByBaseType but without char (cause vigra doesn't like it :-))
  template<template<typename> class F, typename ... ArgTypes> void callByBaseType_flexmav(BaseType type, ArgTypes & ... args)
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
  
  template<template<typename> class F, typename R, typename ... ArgTypes> R callByBaseType_flexmav_r(BaseType type, ArgTypes & ... args)
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
  
  template<uint DIM> class FlexMAV;
  template <uint DIM, typename T, uint IDX> vigra::MultiArrayView<DIM,T> getFixedChannel(FlexMAV<DIM> &a);
  
  template<uint DIM> class FlexMAV {
  public:
    
    typedef vigra::TinyVector<vigra::MultiArrayIndex, DIM> difference_type;
    
    FlexMAV() {};
    FlexMAV(const difference_type &shape, BaseType type, std::vector<cv::Mat> inputs) { create(shape,type,inputs); };
    
    template<typename T> class new_dispatcher {
    public:
      void * operator()(FlexMAV<DIM> &mav, cv::Mat &input)
      {
        return new vigra::MultiArrayView<DIM,T>(mav.shape(), (T*)input.data);
      }
    };
    
    template<typename T> class delete_dispatcher {
    public:
      void operator()(FlexMAV<DIM> &mav)
      {
        vigra::MultiArrayView<DIM,T> *img = mav.template get<T>();
        delete img;
      }
    };
    
    /*template<typename T> class reshape_dispatcher {
    public:
      void operator()(FlexMAV<DIM> &mav, difference_type &shape)
      {
        vigra::MultiArrayView<DIM,T> *img = mav.template get<T>();
        mav->reshape(shape);
      }
    };*/
    
    void create(difference_type shape, BaseType type, cv::Mat &input)
    {
      if (_data)
        call<delete_dispatcher>(*this);
      _shape = shape;
      _type = type;
      _mat = input;
      _data = call_r<new_dispatcher,void*>(*this, input);
    }
    
    void create(difference_type shape, BaseType type)
    {
      if(_type == type || _shape == shape)
        return;

      if (_data)
        call<delete_dispatcher>(*this);
      //TODO n-d!
      int size[DIM];
      for (int i = 0; i < DIM; i++)
        size[i] = shape[i];
      _mat = cv::Mat(DIM, size, BaseType2CvDepth(_type));
      _data = call_r<new_dispatcher, void *>(*this, _mat);
    }
    
    void reshape(difference_type shape)
    { 
      create(shape, _type);
    }
    
    template<template<typename> class F, typename ... ArgTypes> void call(ArgTypes & ... args) { callByBaseType_flexmav<F>(_type, args...); }
    template<template<typename> class F, typename R, typename ... ArgTypes> R call_r(ArgTypes & ... args) { return callByBaseType_flexmav_r<F,R>(_type, args...); }
    
    
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