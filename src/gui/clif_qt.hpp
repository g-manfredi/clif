#ifndef _CLIFQT_H
#define _CLIFQT_H

#include "clif.hpp"
#include "subset3d.hpp"

#include <QImage>

#include "config.h"

namespace clif {
  
/** \defgroup clif_cv OpenCV Bindings
*  @{
*/
CLIF_EXPORT void readQImage(Datastore *store, const std::vector<int> idx, QImage &img, int flags);
CLIF_EXPORT void readEPI(clif::Subset3d *set, QImage &img, int line, double disp, int flags = 0);
/**
 *  @}
 */
    
}

#endif