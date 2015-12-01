File Format {#file_format}
======

[TOC]

# File Format {#file_format}

THE CLif file format is based on HDF5 which use an hierachical organisation which conatins attributes and datasets. In clif the terminology is a it different as *dataset* refers to a whole tree within a HDF5 file (all data and meta-data necessary to interpret the light field dataset) and a *datastore* refers to a single HDF5 dataset. Attributes are used just the same. The following is an example of such a dataset:

dataset "default":
~~~~~~~~~~~~~~~~
calibration
  extrinsics              
    default              
        data              -> data
        world_to_camera   [-1,0,0,0,0,-500]
        type              LINE
        camera_to_line_start[0,0,0,-50,0,0]
        line_step         [1.04,0,0]
  imgs                    
    checkers             
        type              CHECKERBOARD
        size              [8,5]
        data              1536 x 2160 x 3 x 22
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
        img_points           81 x 3

        world_points         81 x 3
  data                       1536 x 2160 x 3 x 81
~~~~~~~~~~~~~~~~

## Organisation

### Attribute Groups

In most situations a group of attributes together describe some part in the files. For example intrinsic calibration data (line 14) needs information about the projection, projection center and distortion, therefore those attributes are always found in a single group.

### Alternatives

In Many places it is possible to store multiple alternating sets of attributes. In such a case the structure allows for one additional level of indirection under the relevant root. In the example above *carlibation/extrinsics/default/* is one alternative for *extrinsics*, but the group *carlibation/extrinsics/rudis_special_calibration* could exist at the same time. At the moment the first alternative is automatically selected most of the time, but a priority system is planned.

### Dataset Root

A single CLIF file allows the storage of several datasets. The root for all datasets is */clif/* under which a number of datasets each have their own root, the example above was stored under */clif/default/* therefore the actual tree as printed by `h5lr -r` is:
~~~~~~~~~~~~~~~~
/                        Group
/clif                    Group
/clif/default            Group
/clif/default/calibration Group
/clif/default/calibration/extrinsics Group
/clif/default/calibration/extrinsics/default Group
/clif/default/calibration/extrinsics/default/data Soft Link {/clif/default/data}
/clif/default/calibration/imgs Group
/clif/default/calibration/imgs/checkers Group
/clif/default/calibration/imgs/checkers/data Dataset {22/Inf, 3, 2160, 1536}
/clif/default/calibration/intrinsics Group
/clif/default/calibration/intrinsics/checkers Group
/clif/default/calibration/intrinsics/checkers/source Soft Link {/clif/default/calibration/mapping/checkers}
/clif/default/calibration/mapping Group
/clif/default/calibration/mapping/checkers Group
/clif/default/calibration/mapping/checkers/source Soft Link {/clif/default/calibration/imgs/checkers}
/clif/default/data       Dataset {81/Inf, 3, 2160, 1536}
~~~~~~~~~~~~~~~~

### Soft Links

When new groups are derived from existing data within the file it is desirable to be able to specify this dependecy in case later processing steps need access to the input data. This is realised with soft links visible for example in line 16 or 23.

### External Links

In many cases when working with large datasets it is neither desired to append/write to an existing CLIF, however copying a large dataset for small processing steps is also not an option. For these cases *external links* are used which link parts of a dataset into other files. This means that a new file is generated which is actually only a hull containing links to the original file, together with any newly added data. This allows very efficient and organized work on large datasets. Note that both *soft links* as well as *external links* are handled transparently for the end user.

### Multi-Dimensional Data

Just like HDF5, CLIF can store (almost) arbitrary multi dimensional datasets. In the listing above 1 and 2D data is immediately displayed while higher dimensions are simply indicated by giving their size (e.g. 1536 x 2160 x 3 x 81 in line 28).
