[TOC]

# The CLIF Library {#library}


The following will give a brief overview over the core classes used in the clif library and show usage with a few code snippets.

## Namespace and Headers {#headers} 


All parts of the library are accessible within the [clif](@ref clif)  namespace, depending on which parts are required the following headers should be included:


Header       | Provides
-------------|---------
clif.hpp     | core classes, will be included from any of the following headers
subset3d.hpp | Handling of 3D subsets of clif files, like EPI generation
calib.hpp    | Calibration routines
clif_cv.hpp  | OpenCV convenience functions, read contents as cv::Mat, get imgsize as cv::Size, etc...
clif_qt.hpp  | The Same for QT


## Core Classes {#coreclasses}

Class                        | Function
-----------------------------|---------
[Datastore](@ref clif::Datastore)  | gives easy access to HDF5 datasets stored within the clif file, like read/write images, etc...
[Dataset](@ref clif::Dataset)      | clif dataset handling, each clif file may store one or more datasets, the Dataset class provides access, attribute handling and interpretation of several attributes (like image format, interpetation of calibration attributes, ...) 
[ClifFile](@ref clif::ClifFile)    | Represents a clif file
[Attributes](@ref clif::Attributes) and [Attribute](@ref clif::Attribute) | Provide attribute handling and interpretation through templated get/set functions wich support c style pointer arrays, std::vector and enum types. Mostly accessed trough [Dataset](@ref Dataset)

The the [Dataset](@ref clif::Dataset) class is the main way trough which clif files are accessed and handled. It derives from [Datastore](@ref clif::Datastore) to give access to the main image store which store the actual light field, as well as from [Attributes](@ref clif::Attributes) for attribute handling.

The following will mostly be a list of examples to show how to access clif files, see the respective class documentation for more detailed information!.

## Library Design {#design}

The [clif](@ref fileformat) file format in many places allows the storage of several alternatives datasets/calibrations/image-sets. To simplify access in this case, most functions allow to specify the respective selection via the name, if an empty string is passed the selection is done automatically (at the moment the fist is choosen, [TODO](@ref todo_priority) for later: introduce priorities).

Most functionality is directly implemented in the library, for example [readEPI](@ref clif::readEPI) can automatically derive extrinsics from the attributes stored in the file and no manual access is necessary by a library user.
However if direct inspection of attributes is desired the templated [getAttribute](@ref clif::Attributes::getAttribute) provide easy, type-safe access. 
The attribute path in normally specified using boost::filesystem::path.

## Examples {#examples}

The following examples show several use-cases, together with links to the respective documentation.

### Open CLIF file and get a dataset

~~~~~~~~~~~~~{.cpp}
#include "clif.hpp"

using namespace clif;

...

ClifFile f("somefile.clif", H5F_ACC_RDONLY);

Dataset *set = f.openDataset();
~~~~~~~~~~~~~


### Read attributes from dataset {#getattribute_example}

~~~~~~~~~~~~~{.cpp}
using namespace clif;

Dataset *set;

...

vector<double> focal_length_vector;  //focal length in pixels
double focal_length_ptr[2];          //focal length in pixels
double pixel_pitch;

set->getAttribute("calibration/intrinsics/calib1/projection", focal_length_vector);
set->getAttribute("camera_info/pixel_pitch", &pixel_pitch);

// alternative access using pointer:
set->getAttribute("camera_info/pixel_pitch", &pixel_pitch);

printf("focal length in mm: %f %f\n", focal_length_vector[0]*pixel_pitch, focal_length_vector[1]*pixel_pitch);

...

//enum type
CalibPattern pattern;

//getAttribute automatically parses the stored attribute to get enum type
set->getAttribute("calibration/images/sets/calib1/type", pattern);
~~~~~~~~~~~~~

### select first alternative from a set



[getAttribute]: http://www.example.com "Optional title"

~~~~~~~~~~~~~{.cpp}
...
set->getAttribute(set->subGroupPath("calibration/intrinsics)/"projection", focal_length);
...
~~~~~~~~~~~~~


### read image from a dataset 

~~~~~~~~~~~~~{.cpp}
Mat img;

//first image has index 0
readCvMat(set, 0, img);
...
~~~~~~~~~~~~~


### read epi image

~~~~~~~~~~~~~{.cpp}
using namespace clif;
...

Mat img;
Subset3d subset = new Subset3d(set);

//epi for line 42
//use depth of 300mm
subset->readEPI(img, 42, 300, ClifUnit::MM);

//use disparity of 5.3 pixels (ClifUnit::PIXELS is the default)
subset->readEPI(img, 42, 5.3);
...
~~~~~~~~~~~~~