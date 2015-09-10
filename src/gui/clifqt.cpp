#include "clifqt.hpp"

#include "subset3d.hpp"

namespace clif_qt {
  
  using namespace clif_cv;
  using namespace cv;
  
  QImage  cvMatToQImage( const cv::Mat &inMat )
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

          return image.rgbSwapped();
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
  
  void readQImage(Datastore *store, uint idx, QImage &img, int flags)
  {
    Mat m;
    readCvMat(store, idx, m, flags | CLIF_CVT_8U);
    
    //FIXME zero copy memory handling?
    img = cvMatToQImage(m).copy();
  }
  
  void readEPI(clif::Subset3d *set, QImage &img, int line, double depth, int flags)
  {
    Mat m;
    set->readEPI(m, line, depth, flags | CLIF_CVT_8U);
    
    //FIXME zero copy memory handling?
    img = cvMatToQImage(m).copy();
  }
  
}