#ifndef _CLIF_CAM_H
#define _CLIF_CAM_H

#include "tree_derived.hpp"

namespace clif {

class DepthDist : public Tree_Derived
{
public:
  DepthDist(const cpath& path, double at_depth);
  
  virtual bool load();
  virtual bool operator==(const Tree_Derived & rhs) const;

private:
  double _depth = -1.0;
};

}

#endif