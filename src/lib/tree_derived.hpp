#ifndef _CLIF_TREE_DERIVED
#define _CLIF_TREE_DERIVED

#include "core.hpp"

namespace clif {
  
class Dataset;

//FIXME introduce dataset/group time stamp to sync?
class Tree_Derived {
public:  
  Tree_Derived(const cpath& path);
  
  const cpath & path() const;
  
  //actually initialize class
  virtual bool load(Dataset *set) = 0;
  
  //inherited classes need to check if rhs has same type first!
  virtual bool operator==(const Tree_Derived & rhs) const = 0;
  
  virtual Tree_Derived* clone() const = 0;

protected:
  cpath _path;
};

template <typename D> class Tree_Derived_Base: public Tree_Derived {
public:
    Tree_Derived_Base(const cpath& path) : Tree_Derived(path) {};
  
    Tree_Derived* clone() const
    {
        return new D(dynamic_cast<D const&>(*this));
    }
};

} //namespace clif

#endif