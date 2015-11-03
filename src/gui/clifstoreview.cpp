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
  _store = store;
  _vbox = new QVBoxLayout(this);
  setLayout(_vbox);
  
  _view = new clifScaledImageView(this);
  _vbox->addWidget(_view);
  
  _slider = new QSlider(Qt::Horizontal, this);
  _slider->setTickInterval(1);
  _slider->setTickPosition(QSlider::TicksBelow);
  _slider->setMaximum(_store->imgCount()-1);
  _vbox->addWidget(_slider);
  
  _qimg = new QImage();
  
  _show_idx = 0;
  load_img();
  
  connect(_slider, SIGNAL(valueChanged(int)), this, SLOT(queue_sel_img(int)));
}

clifStoreView::~clifStoreView()
{
  delete _qimg;
}

void clifStoreView::queue_sel_img(int n)
{
  _show_idx = n;

  if (!_timer) {
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(load_img()));
    _timer->setSingleShot(true);
    if (!_rendering)
      _timer->start(0);
  }
}

void clifStoreView::load_img()
{
  if (_timer)
    _timer = NULL;
  
  if (_curr_idx == _show_idx)
    return;
  
  std::vector<int> n_idx(_store->dims(),0);
  n_idx[3] = _show_idx;
  
  readQImage(_store, n_idx, *_qimg, 0);
  _view->setImage(*_qimg);
  
  _curr_idx = _show_idx;
  
  //force results of this slow operation to be displayed
  _rendering = true;
  qApp->processEvents();
  _rendering = false;
  
  if (_timer)
    _timer->start(0);
}


}