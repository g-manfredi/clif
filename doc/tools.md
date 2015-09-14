[TOC]

# Tools and File Handling {#tools}


## Ini Type and Attribute specification {#ini}


### Attributes  {#tools_attr_ini}

~~~~~~~~~~~~~
[format]

type         = UINT16
organisation = BAYER_2x2
order        = RGGB

[calibration.extrinsics.default]
type = LINE

world_to_camera = -1 0 0   0 0 -500
camera_to_line_start = 0 0 0  -50 0 0
line_step = 1.04 0 0
~~~~~~~~~~~~~

### Type Specification {#ini_type_system}

Ini files are used to specify type information and can be passed to the [clif](@ref tool_clif) tool (using -t):

~~~~~~~~~~~~~
[format]

type         = ENUM UINT8 UINT16
organisation = ENUM PLANAR INTERLEAVED BAYER_2x2
order        = ENUM RGGB BGGR GBRG GRBG RGB

[camera_info]

pixel_pitch = DOUBLE

[lens_info]

focal_length = DOUBLE
focus_distance = DOUBLE

;arbitrary names
;calibration images may be saved under calibration.data - same format as main dataset
[calibration.intrinsics.*]

; distortion model type
; CV8 - opencv style eight parameter distortion model
; LINES - 3D lines gridded projection - may be used to fit 
type = ENUM CV8 LINES

; offset in pixels
projection = DOUBLE
; focal length in pixel
projection_center = DOUBLE

opencv_distortion = DOUBLE

lines_sizes = INT
lines_offset = DOUBLE
lines_direction = DOUBLE

[calibration.extrinsics.*]
; the range of images from .data [start, end)
; exmaple with 101 images
; range = 0 101
range = INT
type = ENUM LINE

;means = DESIGN
means = ENUM DESIGN

; two coordinate systems: world, virtual plenoptic camera 
; 6 coordinates, three rotation (axis–angle representation), three translation
; for simplicity the rotation vector is scaled by pi/2 e.g (0 0 1) is a 90° rotation around z axis
; normally world is specified with x,y as floor and z as heigth
; camera with x, y parallel to imaging plane, and positive z into the world
; example: plenoptic camera 500 mm from world origin along y axis (in world coordinates) respectively along z axis (in camera coordinate system)
; world_to_camera = -1 0 0   0 0 -500
world_to_camera = DOUBLE

; the start of the line
;camera_to_line_start = 0 0 0  50 0 0
camera_to_line_start = DOUBLE
; line_step = -1 0 0
line_step = DOUBLE

; calibration.images.data contains calibration images
[calibration.images.sets.*]

range = INT
type = ENUM CHECKERBOARD HDMARKER

;size of 2d grid (checkerboard/markers)
size = INT

hdmarker_recursion = INT

;flat target calibration points:
;pointdata = FLOATs the detected points - array of floats img.x, img.y, world.x, world.y
;pointcounts = INTs describes the vector of vectors in pointdata for each image the number of points found
~~~~~~~~~~~~~


## Command Line Tools {#cli_tools}


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

- Create dataset from images an ini file:
~~~~~~~~~~~~~
clif -i import.ini *.tif -o set.clif
~~~~~~~~~~~~~


- Add calibration images
~~~~~~~~~~~~~
clif -i calib.ini *.tif -o set.clif
~~~~~~~~~~~~~


- Extract images
~~~~~~~~~~~~~
clif -i set.clif -o img%06d.jpg
~~~~~~~~~~~~~

- detect calibration pattern and store result in same file
~~~~~~~~~~~~~
clif -i set.clif -o set.clif --detect-patterns
~~~~~~~~~~~~~

- execute opencv-camera calibration (with 8-parameter distortion model) and store everything in a second file
~~~~~~~~~~~~~
clif -i set.clif -o calibrated_set.clif --opencv-calibrate
~~~~~~~~~~~~~
