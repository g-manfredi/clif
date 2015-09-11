#ifndef _CLIF_CORE_H
#define _CLIF_CORE_H

#include <boost/filesystem.hpp>

#include "helpers.hpp"
#include "hdf5.hpp"
#include "enumtypes.hpp"

enum class ClifUnit : int {INVALID,MM,PIXELS};

namespace clif {
  
using boost::filesystem::path;


/*
 * \cond invalid type for BaseType lookup
 */
struct InvalidBaseType {};
/*
 * \endcond
 */
  
//base type for elements
enum class BaseType : int {INVALID,INT,FLOAT,DOUBLE,STRING};

static std::type_index BaseTypeTypes[] = {std::type_index(typeid(InvalidBaseType)), std::type_index(typeid(int)), std::type_index(typeid(float)), std::type_index(typeid(double)), std::type_index(typeid(char))};

static std::unordered_map<std::type_index, BaseType> BaseTypeMap = { 
    {std::type_index(typeid(char)), BaseType::STRING},
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
    case BaseType::INT : F<int>()(args...); break;
    case BaseType::FLOAT : F<float>()(args...); break;
    case BaseType::DOUBLE : F<double>()(args...); break;
    case BaseType::STRING : F<char>()(args...); break;
    default:
      abort();
  }
}

template<template<typename> class F, typename R, typename ... ArgTypes> R callByBaseType(BaseType type, ArgTypes ... args)
{
  switch (type) {
    case BaseType::INT : return F<int>()(args...); break;
    case BaseType::FLOAT : return F<float>()(args...); break;
    case BaseType::DOUBLE : return F<double>()(args...); break;
    case BaseType::STRING : return F<char>()(args...); break;
    default:
      abort();
  }
}

int combinedTypeElementCount(DataType type, DataOrg org, DataOrder order);
int combinedTypePlaneCount(DataType type, DataOrg org, DataOrder order);

H5::PredType H5PredType(DataType type);

}

#endif