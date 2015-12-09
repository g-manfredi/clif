#ifndef _CLIF_TREE_DERIVED
#define _CLIF_TREE_DERIVED

#include "core.hpp"

namespace clif {

//FIXME introduce dataset/group time stamp to sync?
class Tree_Derived {
public:  
  Tree_Derived(const cpath& path);
  
  const cpath & path() const;
  
  //actually initialize class
  virtual bool load() = 0;
  
  //inherited classes need to check if rhs has same type first!
  virtual bool operator==(const Tree_Derived & rhs) const = 0;

private:
  cpath _path;
};

} //namespace clif

#endif