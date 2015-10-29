#ifndef _CLIF_HDF5_H
#define _CLIF_HDF5_H

#include <H5Cpp.h>

#include "enumtypes.hpp"
#include "helpers.hpp"
#include "clif.hpp"

#include <boost/filesystem.hpp>

namespace clif {

bool h5_obj_exists(H5::H5File f, const char * const path);
bool h5_obj_exists(H5::H5File f, const std::string path);
bool h5_obj_exists(H5::H5File f, const boost::filesystem::path path);

void h5_create_path_groups(H5::H5File &f, boost::filesystem::path path);

ClifFile h5_memory_file();

std::vector<std::string> listH5Datasets(H5::H5File &f, std::string parent);

BaseType PredType_to_native_BaseType(H5::PredType type);
BaseType hid_t_to_native_BaseType(hid_t type);
H5::PredType BaseType_to_PredType(BaseType type);
H5::PredType H5PredType(BaseType type);
}

#endif