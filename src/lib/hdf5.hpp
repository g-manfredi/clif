#ifndef _CLIF_HDF5_H
#define _CLIF_HDF5_H

#include <H5Cpp.h>

#include "helpers.hpp"

#include <boost/filesystem.hpp>

namespace clif {

bool h5_obj_exists(H5::H5File &f, const char * const path);
bool h5_obj_exists(H5::H5File &f, const std::string path);
bool h5_obj_exists(H5::H5File &f, const boost::filesystem::path path);

void h5_create_path_groups(H5::H5File &f, boost::filesystem::path path);

H5::H5File h5_memory_file();

std::vector<std::string> listH5Datasets(H5::H5File &f, std::string parent);

}

#endif