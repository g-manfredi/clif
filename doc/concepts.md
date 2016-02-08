Concepts and Usage
================

[TOC]


# Overview {#over}

The clif library implements data and metadata storage and dataset handling as well as simple (image) processing methods transparently applied at the time of access (like undistortion, format conversion, demosaicing and similar).

## Terminology {#concepts_term}

- **path**: a path within a clif/hdf5 file is in meaning identical to paths in a filesystem
- **attribute**: An named entity in a clif file which contains an N-dimensional array (with N >= 0), for metadata use
- **group**: A entity in a clif/hdf5 file with the same meaning as a directory in a filesystem
- **dataset (clif)**: a tree unter the root /clif/name, where *name* is the name of the dataset
- **datastore (clif)**: hdf5 dataset - all data which is not metadata (e.g. all data which does not solely serve to interpret other parts of the file) are stored in *datastores*
- **dataset (hdf5)**: an N-dimensional array stored under a path and accessible as a whole or in parts, this contains the data to be processed (e.g. images)
- **softlink**: a special node in the tree structure of a clif/hdf5 file which point to an other path within the file (which may or may not actually exist), used to avoid duplication within a file and to signify derivations/processing chains within a file. 
- **external link**: A special node in an hdf5 file which points to a path in a *different* file, within clif this is used solely for datastores.

Links are used transparently within the clif library - users do not normally need to be aware when using them.

## image handling {#concepts_img}

Within clif images are interpreted as 3D arrays, with the third dimension being color. This means that a continuous 2d slice from an image is a continuous 2d image and any method working on 2d slices may trivially be extendend to color or multispectral images. This also means that most image processing methods should work on 3d arrays by internally iterating the third dimension and processing each 2d slice the same.

# Files {#concepts_files}

## example file structure {#concepts_file_ex}


Clif uses hdf5 for storage which means that arbitrary n-dimensional datasets may be stored under a tree-structure. An example file could look like this:

```
clif
  default
    calibration                
      extrinsics              
          default              
            data              -> data                  // soft link
            world_to_camera   [-1,0,0,0,0,-500]        // attribute: 1d arrray
            type              LINE                     // attribute: string (also a 1d array)
            camera_to_line_start[0,0,0,-50,0,0]
            line_step         [1.04,0,0]
      imgs                    
          checkers             
            type              CHECKERBOARD
            size              [8,5]
            data              1536 x 2160 x 3 x 22     //4-d datastore - 22 images of size 1536x2160 with 3 color channels
      intrinsics              
          checkers             
            source            -> calibration/mapping/checkers
            projection        [1907.35,1908.23]
            projection_center [773.234,1068.11]
            type              CV8
            opencv_distortion [-0.0731169,-0.0448424,-0.000689215,-0.000424101,0.349493]
      mapping                 
          checkers                                                                                                                                     
            source            -> calibration/imgs/checkers                                                                                            
            img_size          [1536,2160]                                                                                                             
            img_points        :1 :22                   //2d attribut (1x22)                                                          
            world_points      :1 :22                                                                                                                  
    disparity                                                                                                                                          
      default                 
          data                 1536 x 2160 x 1 x 81
    data                       1536 x 2160 x 3 x 81
```

## file format {#concepts_file_format}

CLIF files with the extension .clif are simply hdf5 files which follow specific conventions to allow easy interpretation of content. For example light field configuration (camera calibration/extrinsics/movement/layout) are described under the group calibration/extrinsics/name/key, where **keys** can be anything hdf5 allows (groups/datasets/attributes/links) and specific have a predefined meaning (see TODO) for all predefined keys. **name** can be an arbitrary string which means several groups may exist, without collision. At the momement the first group is always used for all processing but at a later point selection of different groups and/or some priority system and search functions may be added.

Note that the structure above lives under the root /clif/dataset_name, where **dataset_name** is once again arbitrary and the /clif root signifies that interpretation is according to the clif conventions.

## file handling workflow {#concepts_file_handling}

CLIF makes extensive use of links within its structure, both soft links as well as external links. This allows it to use clif in a non-destructive way. Processing with clif should always follow a workflow where a single input clif file is used and a single output clif file is produced. The output file will contain, in addition to the newly generated content, all previous content of the input clif file. Due to the use of external links datasets will not be copied to the new output but only linked, which means that all information available in the input file will also be acessible trough the output file. When new groups are generated in a clif file by using data from an already existing group then this should be signified by adding a *source* soft-link to the newly generated group which links to the group used as input. Also all releveant parameters useful to a component reading the new group should be stored in the new group. As an example see *calibration/mapping/checkers/source* above.


# File and Data handling {#concepts_hndl}

## importing and command line file handling {#concepts_hndl_import}

The *clif* command line tool can read three types of inputs and store them in clif files:
- images (via OpenCV)
- ini files
- clif files

For example a new file can be generated from an ini file like this:
```
clif -i attributes.ini -o example.clif
```

Or attributes in a dataset might be modified:
```
clif -i attributes.ini example.clif -o example.clif
```

Images can also be added, and will by default land in the 4D datastore /data
```
clif -i attributes.ini *.tif -o example.clif
```

Images may also be stored into a different store with arbitrary dimensions, example for a 5D store (in this case dimension 5 has size 1):
```
clif -o example.clif --store path/to/a/store 5 *.tif
```

## ini files {#concepts_hndl_ini}

In ini files attributes can be set to single values or with an 1-d array:
```
[calibration.extrinsics.default]
type = LINE

world_to_camera = -1 0 0   0 0 -500
camera_to_line_start = 0 0 0  -50 0 0
line_step = 1.04 0 0

data = ->data
```
The last line is as soft link which makes /calibration/extrinsics/default/data point to /data

Note that the type of an element should be known beforehand, a special ini is used to specifiy these types (see openlf/data/types.ini). This file is compiled in but can also be modified/extended and then be passed via -t to change/add types at runtime.

# Core Classes {#concepts_core}

## datatype handling {#concepts_core_types}

Several datatypes are specifically handled in clif using clif::BaseType which allows transparent handling of them whithin c++ code using templates and a special dispatch functionality:

- UINT8
- UINT16
- INT (signed 32-bit)-
- UINT32
- FLOAT
- DOUBLE
- CV_POINT2F (float 2d point)
- STRING
- variable length arrays of any of type above

Any such type may be stored in a matrix/multidimensional array an will be written/read from clif files.
Type handling is achieved using the **BaseType** enum, and template functionality look like this (see TODO) for more details:

~~~~~~~~~~~~~{.cpp}
template<typename T> class some_templated_functor {
public:
  int operator()(X arg1, Y arg2)
  {
    //your code using type T
    T variable;
    ...
  }
};

...

BaseType type;
X var1;
Y var2;

call<some_templated_functor>(type, var1, var2);
~~~~~~~~~~~~~

## Idx {#concepts_core_idx}

The idx class simplifies multidimensional array handling by providing a simple api to mix and access individual and ranges of dimensions. A Matrix is always constructed from and Idx (see below) and an Idx may be constructed from an initializer list containing dimensions and ranges. clif::Mat inherits from clif::Idx, therefore a Mat may be used in place of an Idx.

### access {#concepts_core_idx_acc}

Indiviual indices may be accessed using *operator[int]* and ranges with .r(int,int)

~~~~~~~~~~~~~{.cpp}
Idx size = {1600,1200,3};

Mat img1(BaseType::FLOAT, size);
Mat img2;

img2.create(BaseType::FLOAT, {50,50,100});

//these statements are all equivalent
ten_x_img1_1.create(img2.type(), {img1[0],img1[1],img1[2],10});
ten_x_img1_2.create(img2.type(), {size.r(0,2),10});
ten_x_img1_3.create(img2.type(), {size.r(0,-1),10});
ten_x_img1_4.create(img2.type(), {img1.r(0,2),10});
~~~~~~~~~~~~~

### named dimensions {#concepts_core_idx_names}

Individual dimension may be named (TODO: names are not yet stored in clif files) and accessed with the name instead of the dimension number, which can simplify code especially if the number of dimensions is allowed to change.

~~~~~~~~~~~~~{.cpp}
Idx size = {1600,1200,3};
size.names("x","y","channels")

Mat img1(BaseType::FLOAT, size);

//these statements are all equivalent
ten_x_img1_1.create(img2.type(), {img1["x"],img1[1],img1["channels"],10});
ten_x_img1_2.create(img2.type(), {size.r(0,"channels"),10});
...
~~~~~~~~~~~~~

### iterate {#concepts_core_idx_iter}

Two iterators are defined to iterate a single or a range of dimensions from an clif::Idx. Note that accessing data like this is not particularly fast, this is meant for high level code where the loop contains significant processing (e.g bind all images from a Mat and process them with some filter).

~~~~~~~~~~~~~{.cpp}
Mat largestore(BaseType::INT, {100,100,10,20,30});

largestore.names("x","y","channels","cams","views");

//pos is an Idx and will iterate through dimension 3, other dimensions are set to zero at the beginning
for(auto pos : Idx_It_Dim(largestore, 3)) {
...
}

//pos is an Idx and will iterate through dimensions "channel" to "views", lower dimensions are set to zero.
for(Idx pos : Idx_It_Dims(largestore, "channels","views")) {
...
}
~~~~~~~~~~~~~

## The clif Mat type {#concepts_core_mat}

The clif library provides a flexible multidimensional matrix type which is meant to abstract data handling and allow flexible, type independend code. The clif *Mat* type is not templated, meaning type and dimensionality may change at runtime. Usage is similar to the OpenCV cv::Mat type, but the focus lies on the high level (e.g. memory management/io/type handling) and less on actual element access.

### Conversion {#concepts_core_mat_conv}

While direct element access is possible another way to work with a clif::Mat is to convert it to an OpenCV::Mat or to a vigra::MultiArrayView<DIM,TYPE>. Those objects are constructed "shallow" meaning actual memory data is shared with the original clif::Mat.

~~~~~~~~~~~~~{.cpp}
Mat m;

cv::Mat = cvMat(m); //shallow copy!
vigra::MultiArrayView<float, 2> = vigraMAV<float, 2>(m); //also shallow but type and dimensionality must be known (e.g. use within dispatch)
~~~~~~~~~~~~~

### Binding {#concepts_core_mat_bind}

For any N-dimensional Mat a N-1 dimensional sub-matrix may be bound to a single value for some dimension, the new matrix also uses the original memory in background and only maps the sub-matrix.

~~~~~~~~~~~~~{.cpp}
Mat four_d(BaseType::INT, {500,500,30,40});

//map 2d submatrix (*,*,0,12)
Mat slice = four_d.bind(3, 12).bind(2, 0); 
~~~~~~~~~~~~~

### type dispatch {#concepts_core_mat_dispatch}

Mat also implements the type dispatch to call a templated functor with the appropriate type, usage is like this:

~~~~~~~~~~~~~{.cpp}
template<typename T> class some_templated_functor {
public:
  int operator()(X arg1, Y arg2)
  {
    //your code using type T
    T variable;
    ...
  }
};

...

Mat matrix;

matrix.call<some_templated_functor>(var1, var2);
~~~~~~~~~~~~~

### templated matrix type {#concepts_core_mat_temp}

In many cases the element type is fixed or known at compile time, in that case it is possible to avoid the type dispatch by using the templated Mat_<> class:

~~~~~~~~~~~~~{.cpp}
Mat_<float> matrix({1600,1200,3});
~~~~~~~~~~~~~

The regular and the templated Mat type may also be converted back and forth at will - tough wrong conversion to Mat_<> will fail at runtime.

### element access {#concepts_core_mat_acc}

Individual elements from a Mat (or Mat_<>) may be accessed using either *operator()* overload or the *at()* method, where the arguments are integer coordinates or a single Idx. For the regular Matrix type the type has to be known (use within dispatch), for the templated Mat_ it is of course already known:

~~~~~~~~~~~~~{.cpp}
float val;
Mat_<float> m1({1600,1200,3});

Mat m2 = m1; //shallow conversion

//all equivalent
val = m2.operator()<float>(0,0,0); //annoying
val = m2.at<float>(0,0,0); //better
val = m1.at(0,0,0); //even better
val = m1(0,0,0); //thats the way...
~~~~~~~~~~~~~

# Data access {#concepts_acc}

clif knows two ways to access a clif::Datastore. Either interpreted as images, where the first 3 dimension define image x,y,channels, or raw - in that case arbitrary chunks may be read.

## Raw Access {#concepts_acc_raw}

The easiest way to access a store is to read the whole store into a Mat, although this can be inefficient (atm - TODO transparent IO caching).

~~~~~~~~~~~~~{.cpp}
Datastore *store = ...

Mat m;

//read whole store
store->read(m);
~~~~~~~~~~~~~

On the other hand subdims can also be read with clif::Datastore::read_full_subdims:
~~~~~~~~~~~~~{.cpp}
//we assume a 4d store here
Datastore *store = ...

Mat m;

std::vector<int> select = {0,3};
Idx pos = {0,0,0,0};

//read an epi (dim 0 = x, dim 3 = images)
store->read_full_subdims(m, select, pos);
~~~~~~~~~~~~~

## Image Access {#concepts_acc_img}

Images may be accessed using clif::Datastore::readImage(). The clif::Idx has the same dimensionality as the clif::Datastore, but the first 3 indices **must** be set to zero. The remaining indices give the position of the image in the datastore. Postprocessing options may be passed via the clif::ProcData, e.g. DEMOSAIC, UNDISTORT, scale, ...

~~~~~~~~~~~~~{.cpp}
Datastore *store = ...

Idx pos(store.dims()); //all indices initialized to zero

pos[3] = store.extent()[3]/2; //centerview for 4d lf data

cv::Mat img;

//read undistorted (which implies DEMOSAIC if necessary) centerview
store->readImage(pos, &img, ProcData(UNDISTORT));
~~~~~~~~~~~~~

It is also possible to read the whole store into a single clif::Mat with postprocessing:
~~~~~~~~~~~~~{.cpp}
Datastore *store = ...

Mat m;

//read store as demosaiced 8-bit images
store->read(m, ProcData(DEMOSAIC | CVT_8U));
~~~~~~~~~~~~~