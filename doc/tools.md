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

world_to_camera = -1 0 0   0 0 -500
camera_to_line_start = 0 0 0  -50 0 0
line_step = 1.04 0 0
~~~~~~~~~~~~~

extrinsics:
~~~~~~~~~~~~~
[calibration.extrinsics.default]
type = LINE

world_to_camera = -1 0 0   0 0 -500
camera_to_line_start = 0 0 0  -50 0 0
line_step = 1.04 0 0
~~~~~~~~~~~~~

hdmarkers:
~~~~~~~~~~~~~
[calibration.images.sets.hdmarker]

type = HDMARKER
marker_size = 39.64
~~~~~~~~~~~~~

### Type Specification {#ini_type_system}

Ini files are used to specify type information and can be passed to the [clif](@ref tool_clif) tool (using -t):

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
      mobilets             
        type              LINE
        world_to_camera   [-1,0,0,0,0,-500]
        camera_to_line_start[0,0,0,-50,0,0]
        line_step         [1.04,0,0]
  images                  
      sets                 
        checkers          
            type           CHECKERBOARD
            size           [8,5]
            pointdata      [248.593,828.244,0,0,400.939,828.961,1,0,554.742,830.049,2, ... ]
            pointcounts    [40,40,0,40,40,0,40,40,40,0,40, ... ]
  intrinsics              
      checkers             
        type              CV8
        projection        [1905.52,1905.52]
        projection_center [767.5,1079.5]
        opencv_distortion [-0.082782,0.0610704,0,0,0]
format                     
  type                    UINT16
  organisation            INTERLEAVED
  order                   RGB
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

- add calibration images
~~~~~~~~~~~~~
clif -i import.ini set.clif --calib-images *.tif -o set2.clif
~~~~~~~~~~~~~

- Extract images
~~~~~~~~~~~~~
clif -i set2.clif -o img%06d.jpg
~~~~~~~~~~~~~

- detect calibration pattern and store result in same file
~~~~~~~~~~~~~
clif -i set2.clif -o set3.clif --detect-patterns
~~~~~~~~~~~~~

- execute opencv-camera calibration (with 8-parameter distortion model) and store everything in a second file
~~~~~~~~~~~~~
clif -i set3.clif -o calibrated_set.clif --opencv-calibrate
~~~~~~~~~~~~~
