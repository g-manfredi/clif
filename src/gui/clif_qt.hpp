#ifndef _CLIFQT_H
#define _CLIFQT_H

#include "subset3d.hpp"
#include "clif.hpp"

#include <QImage>

#include "config.h"

namespace clif {
  
/** \defgroup clif_cv OpenCV Bindings
*  @{
*/
CLIF_EXPORT QImage clifMatToQImage(const cv::Mat &inMat);
CLIF_EXPORT void readQImage(Datastore *store, const std::vector<int> idx, QImage &img, int flags, double depth = std::numeric_limits<float>::quiet_NaN(), double min = std::numeric_limits<float>::quiet_NaN(), double max = std::numeric_limits<float>::quiet_NaN());
CLIF_EXPORT void readEPI(clif::Subset3d *set, QImage &img, int line, double disp, int flags = 0);
/**
 *  @}
 */
    
}

#endif