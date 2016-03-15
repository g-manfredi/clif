#include "mat_helpers.hpp"
#include "mat.hpp"

namespace clif {

template<typename T> class get_intensities_dispatcher {
public:
  void operator()(Mat* m, Mat* intensities, int x, int y, float disp, int base_view)
  // FIXME Add check for baseview 
  {
   
    if (base_view < 0 || base_view >= (*m)[3])
	throw "base_view is out of range!";
    for(int v = 0; v<(*m)[3];v++)
    {
      float x_real = x + (base_view - v)*disp;
      int x_l = x_real;
      int x_r = x_l + 1;
      float f = x_real - x_l;
      if (x_l < 0 || x_r >= (*m)[0])
	continue;
      for (int c=0; c<(*m)[2]; c++ )	
	intensities->operator()<T>(c,v) = m->operator()<T>(x_l, y, c, v)*(1-f) + m->operator()<T>(x_r, y, c, v)*f; 
      
    }    
  }
};

void get_intensities(Mat& m, Mat& intensities, int x, int y, float disp, int base_view)
{
  intensities.create(m.type(), {m[3],m[2]});
   if (base_view == -1)
	base_view = m[3]/2;
  m.callIf<get_intensities_dispatcher,_is_convertible_to_float>(&m,&intensities,x,y,disp, base_view);
}

}
