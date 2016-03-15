#ifndef _CLIF_M_HELPERS_H
#define _CLIF_M_HELPERS_H

#include "mat.hpp"

namespace clif {
  
void get_intensities(Mat& m, Mat& intensities, int x, int y, float disp, int base_view = -1);

}

#endif
