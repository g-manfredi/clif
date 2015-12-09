#include "cam.hpp"

namespace clif {

DepthDist::DepthDist(const cpath& path, double at_depth)
  : Tree_Derived(path), _depth(at_depth)
{
  
}
  
bool DepthDist::load()
{
 
  return true;
}

bool DepthDist::operator==(const Tree_Derived & rhs) const
{
  const DepthDist * other = dynamic_cast<const DepthDist*>(&rhs);
  
  if (other && other->_depth == _depth)
    return true;
  
  return false; 
}
  
}