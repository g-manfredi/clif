[TOC]

# Tools and File Handling {#tools}


## Ini Type and Attribute specification {#ini}


### Attributes  {#tools_attr_ini}

ini file example for bayer pattern
~~~~~~~~~~~~~
[format]

organisation = BAYER_2x2
order        = RGGB
~~~~~~~~~~~~~

extrinsics:
~~~~~~~~~~~~~
[calibration.extrinsics.default]
type = LINE

data = ->data
world_to_camera = -1 0 0   0 0 -500
camera_to_line_start = 0 0 0  -50 0 0
line_step = 1.04 0 0
~~~~~~~~~~~~~

hdmarkers calibration:
~~~~~~~~~~~~~
[calibration.imgs.hdmarker]

type = HDMARKER
marker_size = 39.64
~~~~~~~~~~~~~


## Command Line Tools {#cli_tools}

most tools reside in src/bin

### clifinfo {#tool_clifinfo}

print contents of a clif file

~~~~~~~~~~~~~
clifinfo -i somefile.clif
~~~~~~~~~~~~~
output:
~~~~~~~~~~~~~
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
~~~~~~~~~~~~~


### clif {#tool_clif}

- import and export metadata and images to/from a clif file
- execute calibration pattern detection
- execute intrinsic calibration


#### examples  {#tool_clif_examples}

- Create dataset from images and ini file:
~~~~~~~~~~~~~
clif -i import.ini *.tif -o set.clif
~~~~~~~~~~~~~

- add calibration images (clif -i [ini-file] [input clif] --store [hdf5 path] [dimensions] [images] -o [output clif])
~~~~~~~~~~~~~
clif -i import.ini set.clif --store calibration/imgs/hdmarker/data 4 *.tif -o set2.clif
~~~~~~~~~~~~~

- Extract images
~~~~~~~~~~~~~
clif -i set2.clif -o img%06d.jpg
~~~~~~~~~~~~~

- detect calibration pattern and store result
~~~~~~~~~~~~~
clif -i set2.clif -o set3.clif --detect-patterns
~~~~~~~~~~~~~

- execute opencv-camera calibration (with 8-parameter distortion model) and store everything in a second file
~~~~~~~~~~~~~
clif -i set3.clif -o calibrated_set.clif --opencv-calibrate
~~~~~~~~~~~~~

The *clif* tool always links datastores of the output file to the input file to avoid copying data if not necessary. You can also combine any of the commands above which will then be executed one after another on the output file. You can also use the same input and output file, in which case processing is performed inplace, e.g. to add calibration images and perform calibration in one step:
~~~~~~~~~~~~~
clif -i set.clif calib.ini -o set.clif --store calibration/imgs/hdmarker/data 4 *.tif --detect-patterns --opencv-calibrate
~~~~~~~~~~~~~
