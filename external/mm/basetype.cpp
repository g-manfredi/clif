#include "basetype.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace clif {

static BaseType BaseTypeAtomicMask = BaseType(int(BaseType::VECTOR)-1);
BaseType BaseTypeMaxAtomicType = BaseType::STRING;

//FIXME generate this list automatically!

static std::type_index BaseTypeSimpleTypes[] = {
  std::type_index(typeid(InvalidBaseType)),
  std::type_index(typeid(uint8_t)),
  std::type_index(typeid(uint16_t)),
  std::type_index(typeid(int)),
  std::type_index(typeid(uint32_t)),
  std::type_index(typeid(float)),
  std::type_index(typeid(double)),
  std::type_index(typeid(char))
};

static std::type_index BaseTypeVectorTypes[] = {
  std::type_index(typeid(InvalidBaseType)),
  std::type_index(typeid(std::vector<uint8_t>)),
  std::type_index(typeid(std::vector<uint16_t>)),
  std::type_index(typeid(std::vector<int>)),
  std::type_index(typeid(std::vector<uint32_t>)),
  std::type_index(typeid(std::vector<float>)),
  std::type_index(typeid(std::vector<double>)),
  std::type_index(typeid(std::vector<char>))
};

BaseType toAtomicBaseType(const BaseType &type)
{
  return type & BaseTypeAtomicMask;
}

std::type_index BaseType2typeid(const BaseType &type)
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
    case CV_32FC2 : return clif::BaseType::CV_POINT2F;
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
    case clif::BaseType::CV_POINT2F : return CV_32FC2;
    default :
      abort();
  }
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

template<typename T> class basetype_max_dispatcher {
public:
  double operator()()
  {
    return std::numeric_limits<T>::max();
  }
};

double BaseType_max(BaseType type)
{
  if (type == BaseType::FLOAT || type == BaseType::DOUBLE)
    return 1.0;
  
  return callIf<double,basetype_max_dispatcher,_is_convertible_to_float>(type);
}

template<> double BaseType_max<float>()
{
  return 1.0;
}

template<> double BaseType_max<double>()
{
  return 1.0;
}

H5::CompType _h5_cv_point2f_native_type()
{
  H5::CompType t(sizeof(cv::Point2f));
  H5Tinsert(t.getId(), "x", HOFFSET(cv::Point2f, x), H5T_NATIVE_FLOAT);
  H5Tinsert(t.getId(), "y", HOFFSET(cv::Point2f, y), H5T_NATIVE_FLOAT);

  return t;
}

H5::CompType _h5_point2f_disk_type()
{
  H5::CompType t(sizeof(cv::Point2f));
  H5Tinsert(t.getId(), "x", HOFFSET(cv::Point2f, x), H5T_IEEE_F32LE);
  H5Tinsert(t.getId(), "y", HOFFSET(cv::Point2f, y), H5T_IEEE_F32LE);
  t.pack();

  return t;
}

//FIXME other types!
H5::DataType toH5DataType(BaseType type)
{  
  if (int(type & BaseType::VECTOR)) {
    H5::DataType t = toH5DataType(type & ~BaseType::VECTOR);
    return H5::VarLenType(&t);
  }
  
  switch (type) {
    case BaseType::UINT8 : return H5::PredType::STD_U8LE;
    case BaseType::UINT16 : return H5::PredType::STD_U16LE;
    case BaseType::INT :    return H5::PredType::STD_I32LE;
    case BaseType::UINT32 :    return H5::PredType::STD_U32LE;
    case BaseType::FLOAT : return H5::PredType::IEEE_F32LE;
    case BaseType::DOUBLE:  return H5::PredType::IEEE_F64LE;
    case BaseType::CV_POINT2F:  return _h5_point2f_disk_type();
    case BaseType::STRING : return H5::PredType::C_S1;
    default :
      assert(type != BaseType::UINT16);
      abort();
  }
}

//FIXME other types!
H5::DataType toH5NativeDataType(BaseType type)
{
  if (int(type & BaseType::VECTOR)) {
    H5::DataType t = toH5NativeDataType(type & ~BaseType::VECTOR);
    return H5::VarLenType(&t);
  }
  
  switch (type) {
    case BaseType::UINT8 :  return H5::PredType::NATIVE_UINT8;
    case BaseType::UINT16 : return H5::PredType::NATIVE_UINT16;
    case BaseType::INT :    return H5::PredType::NATIVE_INT;
    case BaseType::UINT32 :    return H5::PredType::NATIVE_UINT32;
    case BaseType::FLOAT :  return H5::PredType::NATIVE_FLOAT;
    case BaseType::DOUBLE:  return H5::PredType::NATIVE_DOUBLE;
    case BaseType::CV_POINT2F:  return _h5_cv_point2f_native_type();
    case BaseType::STRING : return H5::PredType::C_S1;
    default :
      assert(type != BaseType::UINT16);
      abort();
  }
}

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
            return BaseType::UINT32;
          else
            return BaseType::INT;
      }
    case H5T_FLOAT: 
      if (H5Tget_size(type) == 4)
        return BaseType::FLOAT;
      else if (H5Tget_size(type) == 8)
        return BaseType::DOUBLE;
      break;
    case H5T_VLEN: 
      return BaseType::VECTOR | toBaseType(H5Tget_super(type));
    case H5T_COMPOUND: 
      if (H5Tget_nmembers(type) == 2
          //TODO this matches _any_ float class, like double
          && H5Tget_member_class(type, 0) == H5T_FLOAT
          && H5Tget_member_class(type, 1) == H5T_FLOAT
          && !strcmp(H5Tget_member_name(type, 0), "x")
          && !strcmp(H5Tget_member_name(type, 1), "y"))
          return BaseType::CV_POINT2F;
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
