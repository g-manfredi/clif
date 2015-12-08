#include "clifstoreview.hpp"

#include <QtGui/QtGui>

#include <QApplication>
#include <QSplitter>
#include <QTimer>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>

#include "clifscaledimageview.hpp"
#include "clifepiview.hpp"
#include "clif_qt.hpp"

#include "dataset.hpp"
#include "subset3d.hpp"
#include "preproc.hpp"

namespace clif {

clifStoreView::clifStoreView(Datastore *store, QWidget* parent)
: QWidget(parent)
{
  QHBoxLayout *hbox;
  QVBoxLayout *_vbox;
  QWidget *w;
  QSlider *_slider;
  
  _store = store;
  _vbox = new QVBoxLayout(this);
  setLayout(_vbox);
  
  _view = new clifScaledImageView(this);
  _vbox->addWidget(_view);
  
  ///////////////////// SLIDER /////////////////////
  hbox = new QHBoxLayout(this);
  w = new QWidget(this);
  _vbox ->addWidget(w);
  w->setLayout(hbox);
  
  _sel = new QComboBox(this);
  _sel->addItem("raw", QVariant(0));
  _sel->addItem("demosaic", QVariant(Improc::DEMOSAIC));
  _sel->addItem("gray", QVariant(Improc::CVT_GRAY));
  _sel->addItem("undistort", QVariant(Improc::UNDISTORT));
  _sel->setCurrentIndex(0);
  hbox->addWidget(_sel);
  
  _slider = new QSlider(Qt::Horizontal, this);
  _slider->setTickInterval(1);
  _slider->setTickPosition(QSlider::TicksBelow);
  _slider->setMaximum(_store->imgCount()-1);
  hbox->addWidget(_slider);
  
  ///////////////////// RANGE /////////////////////
  hbox = new QHBoxLayout(this);
  w = new QWidget(this);
  _vbox ->addWidget(w);
  w->setLayout(hbox);
  
  _try_out_new_reader = new QCheckBox("experimental flexibel reader", this);
  connect(_try_out_new_reader, SIGNAL(stateChanged(int)), this, SLOT(rangeStateChanged(int)));
  hbox->addWidget(_try_out_new_reader);
  
  _range_ck = new QCheckBox("scale for range", this);
  connect(_range_ck, SIGNAL(stateChanged(int)), this, SLOT(rangeStateChanged(int)));
  hbox->addWidget(_range_ck);
  
  _sp_min = new QDoubleSpinBox(this);
  _sp_min->setRange(-1000000000,std::numeric_limits<double>::max());
  _sp_min->setDisabled(true);
  connect(_sp_min, SIGNAL(valueChanged(double)), this, SLOT(queue_load_img()));
  hbox->addWidget(_sp_min);
  _sp_max = new QDoubleSpinBox(this);
  _sp_max->setRange(-1000000000,std::numeric_limits<double>::max());
  _sp_max->setDisabled(true);
  connect(_sp_max, SIGNAL(valueChanged(double)), this, SLOT(queue_load_img()));
  hbox->addWidget(_sp_max);
  
  _qimg = new QImage();
  
  _show_idx = 0;
  load_img();
  
  connect(_slider, SIGNAL(valueChanged(int)), this, SLOT(queue_sel_img(int)));
  connect(_sel, SIGNAL(currentIndexChanged(int)), this, SLOT(queue_load_img()));
}

clifStoreView::~clifStoreView()
{
  delete _qimg;
}

void clifStoreView::queue_sel_img(int n)
{
  _show_idx = n;

  queue_load_img();
}

void clifStoreView::rangeStateChanged(int s)
{
  if (s == Qt::Checked) {
    _sp_min->setDisabled(false);
    _sp_max->setDisabled(false);
  }
  else {
    _sp_min->setDisabled(true);
    _sp_max->setDisabled(true);
  }
  
  queue_load_img();
}

void clifStoreView::queue_load_img()
{
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
  
  //FIXME disable for now - add flexibel read configuration to check!
  //if (_curr_idx == _show_idx && _curr_flags == _sel->itemData(_sel->currentIndex()).value<int>() && _range_ck->checkState() != Qt::Checked)
    //return;
  
  _curr_flags = _sel->itemData(_sel->currentIndex()).value<int>();
  _curr_idx = _show_idx;
  
  if (_try_out_new_reader->checkState() == Qt::Checked) {
    Idx pos(_store->dims());
    Idx sub = {0, 3}; //x and imgs -> an epi :-)
    
    pos[1] = _curr_idx;
    
    clif::Mat m;
    
    _store->read_full_subdims(m, sub, pos);
    *_qimg = clifMatToQImage(cvMat(m));
  }
  else {
    std::vector<int> n_idx(_store->dims(),0);
    n_idx[3] = _curr_idx;
    
    if (_range_ck->checkState() == Qt::Checked)
      readQImage(_store, n_idx, *_qimg, _curr_flags, _sp_min->value(), _sp_max->value());
    else
      readQImage(_store, n_idx, *_qimg, _curr_flags);
  }
  
  _view->setImage(*_qimg);
  
  //force results of this slow operation to be displayed
  _rendering = true;
  qApp->processEvents();
  _rendering = false;
  
  if (_timer)
    _timer->start(0);
}


}