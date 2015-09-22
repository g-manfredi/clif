#include "clif_vigra.hpp"

#include <cstddef>
#include <sstream>
#include <string>

namespace clif {

vigra::Shape2 imgShape(Datastore *store)
{
  cv::Size size = imgSize(store);
  
  return vigra::Shape2(size.width, size.height);
}
  
template<typename T> class readimage_dispatcher {
public:
  void operator()(Datastore *store, uint idx, void **raw_channels, int flags = 0, float scale = 1.0)
  {
    //cast
    std::vector<vigra::MultiArrayView<2, T>> **channels = reinterpret_cast<std::vector<vigra::MultiArrayView<2, T>> **>(raw_channels);
    
    //allocate correct type
    if (*channels == NULL)
      *channels = new std::vector<vigra::MultiArrayView<2, T>>(store->channels());
    
    //read
    std::vector<cv::Mat> cv_channels;
    readCvMat(store, idx, cv_channels, flags, scale);
    
    //store in multiarrayview
    for(int c=0;c<(*channels)->size();c++)
      (**channels)[c] = vigra::MultiArrayView<2, T>(imgShape(store), (T*)cv_channels[c].data);
  }
};

void readImage(Datastore *store, uint idx, void **channels, int flags, float scale)
{
  store->call<readimage_dispatcher>(store, idx, channels, flags, scale);
}

void readImage(Datastore *store, uint idx, FlexChannels<2> &channels, int flags, float scale)
{
  std::vector<cv::Mat> cv_channels;
  readCvMat(store, idx, cv_channels, flags, scale);
  
  channels.create(imgShape(store), store->type(), cv_channels);
}

template<typename T> class readepi_dispatcher {
public:
  void operator()(Subset3d *subset, void **raw_channels, int line, double disparity, ClifUnit unit = ClifUnit::PIXELS, int flags = 0, Interpolation interp = Interpolation::LINEAR, float scale = 1.0)
  {
    //cast
    std::vector<vigra::MultiArray<2, T>> **channels = reinterpret_cast<std::vector<vigra::MultiArray<2, T>> **>(raw_channels);
    
    //allocate correct type
    if (*channels == NULL)
      *channels = new std::vector<vigra::MultiArray<2, T>>(subset->dataset()->channels());
    
    //read
    std::vector<cv::Mat> cv_channels;
    subset->readEPI(cv_channels, line, disparity, unit, flags, interp, scale);
    
    vigra::Shape2 shape(cv_channels[0].size().width, cv_channels[0].size().height);
    
    //store in multiarrayview
    for(int c=0;c<(*channels)->size();c++) {
      //TODO implement somw form of zero copy...
      (**channels)[c].reshape(shape);
      (**channels)[c] = vigra::MultiArrayView<2, T>(shape, (T*)cv_channels[c].data);
    }
  }
};

void readEPI(Subset3d *subset, void **channels, int line, double disparity, ClifUnit unit, int flags, Interpolation interp, float scale)
{
  subset->dataset()->call<readepi_dispatcher>(subset, channels, line, disparity, unit, flags, interp, scale);
}

void readEPI(Subset3d *subset, FlexChannels<2> &channels, int line, double disparity, ClifUnit unit, int flags, Interpolation interp, float scale)
{
  std::vector<cv::Mat> cv_channels;
  subset->readEPI(cv_channels, line, disparity, unit, flags, interp, scale);
  
  vigra::Shape2 shape(cv_channels[0].size().width, cv_channels[0].size().height);
  
  channels.create(shape, subset->dataset()->type(), cv_channels);
}

void readEPI(Subset3d *subset, int channel, FlexMAV<2> &img, int line, double disparity, ClifUnit unit, int flags, Interpolation interp, float scale)
{
  std::vector<cv::Mat> cv_channels;
  subset->readEPI(cv_channels, line, disparity, unit, flags, interp, scale);
  
  vigra::Shape2 shape(cv_channels[0].size().width, cv_channels[0].size().height);
  
  img.create(shape, subset->dataset()->type(), cv_channels[channel]);
}

}