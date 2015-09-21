#ifndef _CLIF_ENUMTYPES_H
#define _CLIF_ENUMTYPES_H

#include <unordered_map>
#include <typeindex>
#include <string.h>

namespace clif {
/*
 * \cond invalid type for BaseType lookup
 */
struct InvalidBaseType {};
/*
 * \endcond
 */
  
//base type for elements
enum class BaseType : int {INVALID,UINT8,UINT16,INT,FLOAT,DOUBLE,STRING};

static std::type_index BaseTypeTypes[] = {std::type_index(typeid(InvalidBaseType)), std::type_index(typeid(uint8_t)), std::type_index(typeid(uint16_t)), std::type_index(typeid(int)), std::type_index(typeid(float)), std::type_index(typeid(double)), std::type_index(typeid(char))};

static std::unordered_map<std::type_index, BaseType> BaseTypeMap = { 
    {std::type_index(typeid(char)), BaseType::STRING},
    {std::type_index(typeid(uint8_t)), BaseType::UINT8},
    {std::type_index(typeid(uint16_t)), BaseType::UINT16},
    {std::type_index(typeid(int)), BaseType::INT},
    {std::type_index(typeid(float)), BaseType::FLOAT},
    {std::type_index(typeid(double)), BaseType::DOUBLE}
};
  
template<typename T> BaseType toBaseType()
{
  return BaseTypeMap[std::type_index(typeid(T))];
}
  
//deep dark black c++ magic :-D
template<template<typename> class F, typename ... ArgTypes> void callByBaseType(BaseType type, ArgTypes ... args)
{
  switch (type) {
    case BaseType::UINT8 :  F<uint8_t>()(args...); break;
    case BaseType::UINT16 : F<uint16_t>()(args...); break;
    case BaseType::INT :    F<int>()(args...); break;
    case BaseType::FLOAT :  F<float>()(args...); break;
    case BaseType::DOUBLE : F<double>()(args...); break;
    case BaseType::STRING : F<char>()(args...); break;
    default:
      abort();
  }
}

template<template<typename> class F, typename R, typename ... ArgTypes> R callByBaseType(BaseType type, ArgTypes ... args)
{
  switch (type) {
    case BaseType::UINT8 :  return F<uint8_t>()(args...); break;
    case BaseType::UINT16 : return F<uint16_t>()(args...); break;
    case BaseType::INT :    return F<int>()(args...); break;
    case BaseType::FLOAT :  return F<float>()(args...); break;
    case BaseType::DOUBLE : return F<double>()(args...); break;
    case BaseType::STRING : return F<char>()(args...); break;
    default:
      abort();
  }
}

  //enum class DataType : int {INVALID,UINT8,UINT16};
  enum class DataOrg : int {INVALID,PLANAR,INTERLEAVED,BAYER_2x2};
  enum class DataOrder : int {INVALID,RGGB,BGGR,GBRG,GRBG,RGB};
  enum class ExtrType : int {INVALID,LINE};
  enum class CalibPattern : int {INVALID,CHECKERBOARD,HDMARKER};
  enum class DistModel : int {INVALID,CV8,LINES};
  
  const static char *BaseTypeStr[] = {"INVALID","UINT8","UINT16","INT","FLOAT","DOUBLE","STRING",NULL};
  const static char *DataOrgStr[] = {"INVALID","PLANAR","INTERLEAVED","BAYER_2x2",NULL};
  const static char *DataOrderStr[] = {"INVALID","RGGB","BGGR","GBRG","GRBG","RGB",NULL};
  const static char *ExtrTypeStr[] = {"INVALID","LINE",NULL};
  const static char *CalibPatternStr[] = {"INVALID","CHECKERBOARD","HDMARKER",NULL};
  const static char *DistModelStr[] = {"INVALID","CV8","LINES",NULL};
  
  static std::unordered_map<std::type_index, const char **> enum_mappings = { 
    {std::type_index(typeid(BaseType)), BaseTypeStr},
    {std::type_index(typeid(DataOrg)), DataOrgStr},
    {std::type_index(typeid(DataOrder)), DataOrderStr},
    {std::type_index(typeid(ExtrType)), ExtrTypeStr},
    {std::type_index(typeid(CalibPattern)), CalibPatternStr},
    {std::type_index(typeid(DistModel)), DistModelStr}
  };
  
  template<typename T> const char *enum_to_string(T val)
  {
    const char **strs = enum_mappings[std::type_index(typeid(T))];
    
    return strs[int(val)];
  }
  
  template<typename T> T string_to_enum(const char *str)
  {
    const char **strs = enum_mappings[std::type_index(typeid(T))];
        
    if (!strs)
      throw std::invalid_argument("unknown Enum Type for template!");
      
    for(int i=0;strs[i];i++)
      if (!strcmp(strs[i], str)) {
        return T(i);
        }

    return T(-1);
  }
  
  BaseType CvDepth2BaseType(int cv_type);
  int BaseType2CvDepth(BaseType t);
  
}

#define ClifEnumString(Type,Var) Type ## Str[ static_cast<int>(Var) ]

#endif