#include "enumtypes.hpp"

#include <opencv2/core/core.hpp>

namespace clif {
  
static BaseType BaseTypeAtomicMask = BaseType(int(BaseType::VECTOR)-1);

static std::type_index BaseTypeSimpleTypes[] = {
  std::type_index(typeid(InvalidBaseType)),
  std::type_index(typeid(uint8_t)),
  std::type_index(typeid(uint16_t)),
  std::type_index(typeid(int)),
  std::type_index(typeid(float)),
  std::type_index(typeid(double)),
  std::type_index(typeid(char))
};

static std::type_index BaseTypeVectorTypes[] = {
  std::type_index(typeid(InvalidBaseType)),
  std::type_index(typeid(std::vector<uint8_t>)),
  std::type_index(typeid(std::vector<uint16_t>)),
  std::type_index(typeid(std::vector<int>)),
  std::type_index(typeid(std::vector<float>)),
  std::type_index(typeid(std::vector<double>)),
  std::type_index(typeid(std::vector<char>))
};

BaseType toAtomicBaseType(BaseType &type)
{
  return type & BaseTypeAtomicMask;
}

std::type_index BaseType2typeid(BaseType &type)
{
  if (int(type & BaseType::VECTOR) == 0)
    return BaseTypeSimpleTypes[int(type)];
  else if (int(type & BaseType::VECTOR))
    return BaseTypeVectorTypes[int(toAtomicBaseType(type))];
  else
    abort();
}

//TODO create mappings with cmake?
//FIXME other types!
BaseType CvDepth2BaseType(int cv_type)
{
  switch (cv_type) {
    case CV_8U : return clif::BaseType::UINT8;
    case CV_16U : return clif::BaseType::UINT16;
    case CV_32F : return clif::BaseType::FLOAT;
    default :
      abort();
  }
}

int BaseType2CvDepth(BaseType t)
{
  switch (t) {
    case clif::BaseType::UINT8 : return CV_8U;
    case clif::BaseType::UINT16 : return CV_16U;
    case clif::BaseType::FLOAT : return CV_32F;
    default :
      abort();
  }
}


//FIXME single strings only atm!
int basetype_size(BaseType type)
{ 
  switch (type) {
    case BaseType::STRING : return sizeof(char);
    case BaseType::INT :    return sizeof(int);
    case BaseType::FLOAT : return sizeof(float);
    case BaseType::DOUBLE : return sizeof(double);
    default:
      printf("invalid type!\n");
      abort();
  }
}

BaseType cliini_type_to_BaseType(int type)
{
  switch (type) {
    case CLIINI_STRING : return BaseType::STRING;
    case CLIINI_DOUBLE : return BaseType::DOUBLE;
    case CLIINI_INT : return BaseType::INT;
  }
  
  printf("ERROR: unknown argument type!\n");
  abort();
}


template<typename T> class basetype_size_dispatcher {
public:
  int operator()()
  {
    return sizeof(T);
  }
};

int baseType_size(BaseType t)
{
  return callByBaseType<basetype_size_dispatcher,int>(t);
}

//FIXME other types!
H5::DataType toH5DataType(BaseType type)
{
  switch (type) {
    case BaseType::UINT8 : return H5::PredType::STD_U8LE;
    case BaseType::UINT16 : return H5::PredType::STD_U16LE;
    case BaseType::INT :    return H5::PredType::STD_I32LE;
    case BaseType::FLOAT : return H5::PredType::IEEE_F32LE;
    case BaseType::DOUBLE:  return H5::PredType::IEEE_F64LE;
    case BaseType::STRING : return H5::PredType::C_S1;
    default :
      assert(type != BaseType::UINT16);
      abort();
  }
}

//FIXME other types!
H5::DataType toH5NativeDataType(BaseType type)
{
  switch (type) {
    case BaseType::UINT8 :  return H5::PredType::NATIVE_UINT8;
    case BaseType::UINT16 : return H5::PredType::NATIVE_UINT16;
    case BaseType::INT :    return H5::PredType::NATIVE_INT;
    case BaseType::FLOAT :  return H5::PredType::NATIVE_FLOAT;
    case BaseType::DOUBLE:  return H5::PredType::NATIVE_DOUBLE;
    case BaseType::STRING : return H5::PredType::C_S1;
    default :
      assert(type != BaseType::UINT16);
      abort();
  }
}

int baseType_size(BaseType t);

//FIXME don't only use class? (double vs float , ints...
/*BaseType toBaseType(H5::PredType type)
{    
  switch (type.getClass()) {
    case H5T_STRING : return BaseType::STRING;
    case H5T_INTEGER : return BaseType::INT;
    case H5T_FLOAT: return BaseType::DOUBLE;
    default:
      abort();
  }
  
  printf("ERROR: unknown argument type!\n");
  abort();
}*/

BaseType toBaseType(hid_t type)
{
  switch (H5Tget_class(type)) {
    case H5T_STRING : return BaseType::STRING;
    case H5T_INTEGER :
      switch (H5Tget_size(type)) {
        case 1 : 
          if (H5Tget_sign(type) != H5T_SGN_NONE)
            abort();
          return BaseType::UINT8;
        case 2 :
          if (H5Tget_sign(type) != H5T_SGN_NONE)
            abort();
          return BaseType::UINT16;
        case 3 :
        case 4 :
          if (H5Tget_sign(type) == H5T_SGN_NONE)
            abort();
          return BaseType::INT;
      }
    case H5T_FLOAT: 
      if (H5Tget_size(type) == 4)
        return BaseType::FLOAT;
      else if (H5Tget_size(type) == 8)
        return BaseType::DOUBLE;
      break;
    default:
      printf("ERROR: unknown argument type!\n");
      abort();
  }
}

/*
H5::PredType BaseType_to_PredType(BaseType type)
{
  switch (type) {
    case BaseType::STRING : return H5::PredType::C_S1;
    case BaseType::UINT8 : return H5::PredType::NATIVE_B8;
    case BaseType::UINT16 : return H5::PredType::NATIVE_B16;
    case BaseType::INT : return H5::PredType::NATIVE_INT;
    case BaseType::FLOAT: return H5::PredType::NATIVE_FLOAT;
    case BaseType::DOUBLE: return H5::PredType::NATIVE_DOUBLE;
    default:
    printf("ERROR: unknown argument type!\n");
      abort();
  }
}*/

}