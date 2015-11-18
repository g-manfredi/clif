#include "core.hpp"

namespace clif {

Idx::Idx() : std::vector<int>() {};
Idx::Idx(int size) : std::vector<int>(size) {};

static Idx zeroButOne(int size, int pos, int i)
{
  Idx idx = Idx(size);
  idx[pos] = i;
}

}