#ifndef _CLIF_ENUMTYPES_H
#define _CLIF_ENUMTYPES_H

#include <unordered_map>
#include <typeindex>
#include <string.h>

namespace clif {
  enum class DataType : int {INVALID,UINT8,UINT16};
  enum class DataOrg : int {INVALID,PLANAR,INTERLEAVED,BAYER_2x2};
  enum class DataOrder : int {INVALID,RGGB,BGGR,GBRG,GRBG,RGB};
  enum class ExtrType : int {INVALID,LINE};
  enum class CalibPattern : int {INVALID,CHECKERBOARD,HDMARKER};
  enum class DistModel : int {INVALID,CV8,LINES};
  
  const static char *DataTypeStr[] = {"INVALID","UINT8","UINT16",NULL};
  const static char *DataOrgStr[] = {"INVALID","PLANAR","INTERLEAVED","BAYER_2x2",NULL};
  const static char *DataOrderStr[] = {"INVALID","RGGB","BGGR","GBRG","GRBG","RGB",NULL};
  const static char *ExtrTypeStr[] = {"INVALID","LINE",NULL};
  const static char *CalibPatternStr[] = {"INVALID","CHECKERBOARD","HDMARKER",NULL};
  const static char *DistModelStr[] = {"INVALID","CV8","LINES",NULL};
  
  static std::unordered_map<std::type_index, const char **> enum_mappings = { 
    {std::type_index(typeid(DataType)), DataTypeStr},
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
  
}

#define ClifEnumString(Type,Var) Type ## Str[ static_cast<int>(Var) ]

#endif
