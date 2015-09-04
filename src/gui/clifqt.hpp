#ifndef _CLIFQT_H
#define _CLIFQT_H

#include "clif.hpp"

#include <QImage>

namespace clif_qt {
  
  void readQImage(clif::Datastore *store, uint idx, QImage &img, int flags = 0);
  void readEPI(Clif3DSubset *set, QImage &img, int line, double depth, int flags = 0);
    
}

#endif