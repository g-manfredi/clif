#include "clif_qt.hpp"

#include "dataset.hpp"
#include "subset3d.hpp"

#include "opencv2/highgui/highgui.hpp"

namespace clif {
  
  using namespace cv;
  
  QImage  clifMatToQImage( const cv::Mat &inMat )
  {
    switch ( inMat.type() )
    {
      // 8-bit, 4 channel
      case CV_8UC4:
      {
          QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_RGB32 );

          return image;
      }

      // 8-bit, 3 channel
      case CV_8UC3:
      {
          QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_RGB888 );

          return image;
      }

      // 8-bit, 1 channel
      case CV_8UC1:
      {
          static QVector<QRgb>  sColorTable;

          // only create our color table once
          if ( sColorTable.isEmpty() )
          {
            for ( int i = 0; i < 256; ++i )
                sColorTable.push_back( qRgb( i, i, i ) );
          }

          QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_Indexed8 );

          image.setColorTable(sColorTable);

          return image;
      }

      default:
          std::cout << "::cvMatToQImage() - cv::Mat image type not handled in switch:" << inMat.type() << std::endl;
          break;
    }

    return QImage();
}
  
  void readQImage(Datastore *store, const std::vector<int> idx, QImage &img, int flags, double min, double max)
  {
    cv::Mat img_3d, img_2d;
    store->readImage(idx, &img_3d, flags | CVT_8U, min, max);
    clifMat2cv(&img_3d, &img_2d);
    
    //FIXME zero copy memory handling?
    img = clifMatToQImage(img_2d).copy();
  }
  
  void readEPI(clif::Subset3d *set, QImage &img, int line, double disp, int flags)
  {
    cv::Mat img_3d, img_2d;
    set->readEPI(&img_3d, line, disp, Unit::PIXELS, flags | CVT_8U);
    clifMat2cv(&img_3d, &img_2d);
    
    //FIXME zero copy memory handling?
    img = clifMatToQImage(img_2d).copy();
  }
  
}