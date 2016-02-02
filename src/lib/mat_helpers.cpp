#include "mat_helpers.hpp"
#include "mat.hpp"

namespace clif {



template<typename T> class get_intensities_dispatcher {
public:
  void operator()(const Mat* m, Mat* intensities, int x, int y, float disp)
  {
	for(int v = 0; v<m[3];v++)
	{
		float x_real = x + (m[3]/2] - v)*disp;
		int x_l = x_real;
		int x_r = x_l + 1;
		float f = r_real - x_l;
		if (x_l < 0 and x_r >= m[0])
			continue;
		for (int c=0; c<3; c++ )	
			intensities(v,c) = m(x_l, y, c, v)*(1-f) + m(x_r, y, c, v)*f; 
		
	}    
  }
};

Void get_intensities(const Mat& m, Mat& intensities, int x, int y, float disp)
{
	intensities.create(m.type(), {m[3],m[2]});
	m.callIf<get_intensities_dispatcher,_is_convertible_to_float>(&m,&intensities,x,y,disp);
}
}
