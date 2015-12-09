#include "tree_derived.hpp"

namespace clif {

Tree_Derived::Tree_Derived(const cpath& path)
: _path(path)
{
  
};

const cpath & Tree_Derived::path() const
{
  return _path;
}

}