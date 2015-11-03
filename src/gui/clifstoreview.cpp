#include "clifstoreview.hpp"

#include <QtGui/QtGui>

#include <QApplication>
#include <QSplitter>
#include <QGraphicsLineItem>
#include <QTimer>
#include <QSlider>
#include <QVBoxLayout>

#include "clifscaledimageview.hpp"
#include "clifepiview.hpp"
#include "clif_qt.hpp"

#include "subset3d.hpp"
#include "dataset.hpp"

namespace clif {

clifStoreView::clifStoreView(Datastore *store, QWidget* parent)
: QWidget(parent)
{
  _vbox = new QVBoxLayout(this);
  setLayout(_vbox);
  
  _view = new clifScaledImageView(this);
  _vbox->addWidget(_view);
  
  _slider = new QSlider(Qt::Horizontal, this);
  _slider->setMaximum(store->imgCount());
  _vbox->addWidget(_slider);
  
  std::vector<int> n_idx(store->dims(),0);
  n_idx[3] = 0;
    
  _qimg = new QImage();
  
  readQImage(store, n_idx, *_qimg, 0);
  _view->setImage(*_qimg);
}

clifStoreView::~clifStoreView()
{
  delete _qimg;
}

}