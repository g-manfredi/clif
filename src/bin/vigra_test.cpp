#include <vigra/multi_array.hxx>
#include <vector>

using namespace std;
using namespace vigra;

int main(const int argc, const char *argv[])
{
  int memory;

  
  vector<MultiArrayView<2, int>> array(1);
  
  array[0] = MultiArrayView<2,int>(Shape2(1,1), &memory);
}