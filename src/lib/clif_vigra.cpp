#include "clif_vigra.hpp"

#include <cstddef>
#include <sstream>
#include <string>


#include "opencv2/highgui/highgui.hpp"

namespace clif {

typedef unsigned int uint;

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
      *channels = new std::vector<vigra::MultiArrayView<2, T>>(store->imgChannels());
    
    //read
    std::vector<cv::Mat> cv_channels;

    //readCvMat(store, idx, cv_channels, flags, scale);
    
    //store in multiarrayview
    for(uint c=0;c<(*channels)->size();c++)
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
  //readCvMat(store, idx, cv_channels, flags, scale);
  //FIXME readimage
  abort();
  
  channels.create(imgShape(store), store->type(), cv_channels);
}

template<typename T> class readepi_dispatcher {
public:
  void operator()(Subset3d *subset, void **raw_channels, int line, double disparity, Unit unit = Unit::PIXELS, int flags = 0, Interpolation interp = Interpolation::LINEAR, float scale = 1.0)
  {
    //cast
    std::vector<vigra::MultiArray<2, T>> **channels = reinterpret_cast<std::vector<vigra::MultiArray<2, T>> **>(raw_channels);
    
    //allocate correct type
    if (*channels == NULL)
      *channels = new std::vector<vigra::MultiArray<2, T>>(subset->dataset()->imgChannels());
    
    //read
    std::vector<cv::Mat> cv_channels;
    //subset->readEPI(cv_channels, line, disparity, unit, flags, interp, scale);
    //FIXME readimage
    abort();
    
    vigra::Shape2 shape(cv_channels[0].size().width, cv_channels[0].size().height);
    
    //store in multiarrayview
    for(uint c=0;c<(*channels)->size();c++) {
      //TODO implement somw form of zero copy...
      (**channels)[c].reshape(shape);
      (**channels)[c] = vigra::MultiArrayView<2, T>(shape, (T*)cv_channels[c].data);
    }
  }
};

void readEPI(Subset3d *subset, void **channels, int line, double disparity, Unit unit, int flags, Interpolation interp, float scale)
{
  subset->dataset()->call<readepi_dispatcher>(subset, channels, line, disparity, unit, flags, interp, scale);
}

void readEPI(Subset3d *subset, FlexChannels<2> &channels, int line, double disparity, Unit unit, int flags, Interpolation interp, float scale)
{
  std::vector<cv::Mat> cv_channels;
  //subset->readEPI(cv_channels, line, disparity, unit, flags, interp, scale);
  //FIXME readimage
  abort();
  
  vigra::Shape2 shape(cv_channels[0].size().width, cv_channels[0].size().height);
  
  channels.create(shape, subset->dataset()->type(), cv_channels);
}

void readEPI(Subset3d *subset, int channel, FlexMAV<2> &img, int line, double disparity, Unit unit, int flags, Interpolation interp, float scale)
{
  std::vector<cv::Mat> cv_channels;
      //FIXME readimage
    abort();
  //subset->readEPI(cv_channels, line, disparity, unit, flags, interp, scale);
  
  vigra::Shape2 shape(cv_channels[0].size().width, cv_channels[0].size().height);
  
  img.create(shape, subset->dataset()->type(), cv_channels[channel]);
}

void readEPI(Subset3d *subset, FlexMAV<3> &img, int line, double disparity, Unit unit, int flags, Interpolation interp, float scale)
{
  cv::Mat epi;
  
  subset->readEPI(&epi, line, disparity, unit, flags, interp, scale);
  
  img.create(epi);
  
  assert(img.type() > BaseType::INVALID);
  
  //vigra::Shape3 shape(cv_channels[0].size().width, cv_channels[0].size().height, cv_channels.size());
  
  //int size[3] = {cv_channels[0].size().width, cv_channels[0].size().height, cv_channels.size() };
  
  //cv::Mat img_3d(3, size, cv_channels[0].depth());
  /*cv::Mat img_2d(cv::Size(size[0], size[1]), cv_channels[0].depth());
  
  cv::Range range[3];
  range[0] = cv::Range::all(); 
  range[1] = cv::Range::all(); 
  
  for(int c=0;c<cv_channels.size();c++) {
    range[2] = cv::Range(c,c+1);
    img_2d = cv::Mat(cv_channels[0].size(), cv_channels[0].depth(), img_3d(range).data);
    //cv_channels[c].copyTo(img_2d);
    //memcpy(img_2d.data, cv_channels[c].data, cv_channels[c].elemSize()*cv_channels[c].total());
    memcpy(img_2d.data, cv_channels[c].data, cv_channels[c].elemSize()*cv_channels[c].total());
    cv::imwrite("debug.tiff", img_2d);
  }
  
  memset(img_3d.data, 127, cv_channels[0].elemSize()*cv_channels[0].total()*cv_channels.size());*/
  
  /*for(uint c=0;c<cv_channels.size();c++) {
    memcpy(img_3d.data+c*cv_channels[0].elemSize()*cv_channels[0].total(), cv_channels[c].data, cv_channels[c].elemSize()*cv_channels[c].total());
  }
  
  img.create(shape, subset->dataset()->type(), img_3d);*/
}

}