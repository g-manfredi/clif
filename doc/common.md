# Common options and flags {#common}

[TOC]

## common options for functions accessing image data {#common_imageoptions}

Most functions which in some way directly access light field data allow to pass a number of options which specify the processing before the image data is used.

### `int` flags {#common_flags}

A combination of flags may be passed which specifies image preprocessing or format requirements, the respective actions will automatically be invoked depending on format of input data (e.g. the clif::DEMOSAIC flag will be ignored on non-Bayer image data)

The following (bit-)flags are defined and may be combined with the | operator.

| Flag          | means         |
| ------------- |---------------|
| DEMOSAIC      | demosaic if Bayer pattern encountered |
| CVT_8U        | convert to 8 bit |
| UNDISTORT     | apply undistorition (using currently loaded intrinsics, see clif::dataset::load_intrinsics |
| CVT_GRAY      | convert to grayscale |
| NO_DISK_CACHE | disable the automatic disk cache |

### `float` scale {#common_scale}

A fractional scale may be specififed and is implemented as gaussian  blur + nearest neighbor at the moment.

### `clif::Interpolation` interp {#common_interp}

Select interpolation method, Interpolation::NEAREST and Interpolation::LINEAR are available at the moment.

## disk and memory cache

