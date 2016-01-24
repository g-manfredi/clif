#include "clifstoreview.hpp"
#include "clifstoreview.moc"

#include <QtGui/QtGui>

#include <QObject>
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

  ///////////////// INDICES ////////////////////////

  _dims = store->dims();
  //_show_idx = new std::vector( [_dims];
  _show_idx = new int[_dims];
  for (int i=0; i<_dims; i++) {
    _show_idx[i]=0;
  }
  qDebug() <<"_show_idx";
  for (int i=0; i<_dims; i++) {
    qDebug() <<_show_idx[i];
  }
  
  IndicesHandler * indicesHandler = new IndicesHandler(_dims);
  //values * _val = new values(_dims);
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
 
  QComboBox * _sel1;
  _sel1 = new QComboBox(this);
  QList<QString> *_list;
  _list = new QList<QString>();

  for (int i=0;i<store->dims();i++) {
      _list->append("dimension "+QString::number(i));
  }

  /*
  QList<QString> * _custom_list;
  _custom_list = new QList<QString>();
  _custom_list->append("x");
  _custom_list->append("y");
  _custom_list->append("z");
  _custom_list->append("color");
  _custom_list->append("brightness");
  _custom_list->append("gradient");
  _custom_list->append("further dimensions");
  int custom_dims = _custom_list->size();
  _list = new QList<QString>(*_custom_list);
  */

  
  QStringList *list;
  list = new QStringList(*_list);
  _sel1->addItems(*list);
  hbox->addWidget(_sel1);
  _sel1->setCurrentIndex(2);
  
  //_slider = new Slider_dim(Qt::Horizontal, this);
  _slider = new Slider_dim(Qt::Horizontal);
  _slider->setTickInterval(1);
  _slider->setTickPosition(QSlider::TicksBelow);
  _slider->setMaximum(_store->imgCount()-1);
  _slider->setProperty("Dimension",2);

  connect(_slider, SIGNAL(valueChanged(int)), indicesHandler, SLOT(changeEntry(int)));
  connect(_sel1, SIGNAL(currentIndexChanged(int)), _slider, SLOT(update(int)));//TODO 
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
  _sp_min->setDecimals(10);
  _sp_min->setDisabled(true);
  _sp_min->setMaximumWidth(250);//TODO
  connect(_sp_min, SIGNAL(valueChanged(double)), this, SLOT(queue_load_img()));//TODO
  hbox->addWidget(_sp_min);
  _sp_max = new QDoubleSpinBox(this);
  _sp_max->setRange(-1000000000,std::numeric_limits<double>::max());
  _sp_max->setDisabled(true);
  _sp_max->setDecimals(10);
  _sp_max->setMaximumWidth(250);//TODO
  connect(_sp_max, SIGNAL(valueChanged(double)), this, SLOT(queue_load_img()));//TODO
  hbox->addWidget(_sp_max);
  
  _qimg = new QImage();
  
  //_show_idx = 0;//TODO Why?
  load_img();
  
  connect(_slider, SIGNAL(valueChanged(int)), this, SLOT(queue_sel_img(int)));//TODO
  connect(_sel, SIGNAL(currentIndexChanged(int)), this, SLOT(queue_load_img()));//TODO: better reset to 0 as above
  
  //////////////////// DIMENSIONS /////////////////
  /***************** 1 ***************************/

  w = new QWidget(this);
  _vbox->addWidget(w);
  hbox = new QHBoxLayout(this);
  w->setLayout(hbox);

  QComboBox * _sel2;
  _sel2 = new QComboBox(this);
  _sel2->addItems(*list);
  hbox->addWidget(_sel2);
  _sel2->setCurrentIndex(0);

  /***************** 2 ***************************/

  QComboBox * _sel3;
  _sel3 = new QComboBox(this);
  _sel3->addItems(*list);
  hbox->addWidget(_sel3);
  _sel3->setCurrentIndex(1);

  /////////////////// EXTENDED MODE //////////////////////

  
  w = new QWidget(this);
  _vbox->addWidget(w);
  hbox = new QHBoxLayout(this);
  w->setLayout(hbox);
  QCheckBox *checkbox_ext = new QCheckBox("&Extended mode", this);
  hbox->addWidget(checkbox_ext);

  list_ext * list_ext_obj;
  list_ext_obj = new list_ext(*_list, _sel1->currentIndex(), _sel2->currentIndex(), _sel3->currentIndex());
  
  connect(_sel1, SIGNAL(currentIndexChanged(int)), list_ext_obj, SLOT(change1(int)));
  connect(_sel2, SIGNAL(currentIndexChanged(int)), list_ext_obj, SLOT(change2(int)));
  connect(_sel3, SIGNAL(currentIndexChanged(int)), list_ext_obj, SLOT(change3(int)));
 
  connect(list_ext_obj, SIGNAL(update_1(int)), _sel1, SLOT(setCurrentIndex(int)));
  connect(list_ext_obj, SIGNAL(update_2(int)), _sel2, SLOT(setCurrentIndex(int)));
  connect(list_ext_obj, SIGNAL(update_3(int)), _sel3, SLOT(setCurrentIndex(int)));

  
  
  
  
    for (int i=0;i<store->dims()-3;i++) { 
    //for (int i=0;i<custom_dims-3;i++) { 
    w = new QWidget(this);
    _vbox->addWidget(w);
    hbox = new QHBoxLayout(this);
    w->setLayout(hbox);
    w->setVisible(false);
    connect(checkbox_ext, SIGNAL(clicked(bool)), w, SLOT(setVisible(bool)));

    editable_ComboBox * _sel_ext = new editable_ComboBox();
    _sel_ext->addItems(list_ext_obj->getList());
    hbox->addWidget(_sel_ext);
    _sel_ext->setCurrentIndex(i);
    //_sel_ext->setEnabled(false); 
    _sel_ext->setID(i); //TODO

    connect(list_ext_obj, SIGNAL(list_changed(QStringList)), _sel_ext, SLOT(clear()));
    connect(list_ext_obj, SIGNAL(list_changed(QStringList)), _sel_ext, SLOT(addItems_slot(QStringList)));
    connect(list_ext_obj, SIGNAL(list_changed(QStringList)), this, SLOT(resetIdx()));//TODO

    _slider = new Slider_dim(Qt::Horizontal, this);
    _slider->setTickInterval(1);
    _slider->setTickPosition(QSlider::TicksBelow);
    _slider->setMaximum(_store->imgCount()-1); //TODO
    _slider->setProperty("Dimension",(list_ext_obj->getIndices())[i]);
    connect(_slider, SIGNAL(valueChanged(int)), indicesHandler, SLOT(changeEntry(int)));
    connect(_sel_ext, SIGNAL(currentTextChanged(QString)), list_ext_obj, SLOT(getIndex(QString)));//TODO
    connect(list_ext_obj, SIGNAL(setIndex(int)), _slider, SLOT(update(int)));//TODO
    //connect(_slider, SIGNAL(valueChanged(double)), this, SLOT(queue_load_img()));
    connect(_slider, SIGNAL(valueChanged(int)), this, SLOT(queue_sel_img(int)));
    hbox->addWidget(_slider);

  }

}

clifStoreView::~clifStoreView()
{
  delete _qimg;
}

void clifStoreView::queue_sel_img(int val)
{
  /*_show_idx = n;

  queue_load_img();*/

  if (val >= 0 ) {
    int dim = sender()->property("Dimension").toInt();
    //qDebug() << dim;
    _show_idx[dim] = val;
    _curr_idx = dim;
    //qDebug() << "_show_idx";
    qDebug() <<"_show_idx";
    for (int i=0; i<_dims; i++) {
    qDebug() <<_show_idx[i];
    }
    queue_load_img();
  }

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
  
  _curr_flags = _sel->itemData(_sel->currentIndex()).value<int>();//TODO Change
  //_curr_flags = sender()->Property("Dimension");
  //_curr_idx = _show_idx[]; //TODO Change
  
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
    for (int i=0; i<_store->dims(); i++) {
      n_idx[i] = _show_idx[i];
    }
    //n_idx[3] = _curr_idx;//TODO
    
    if (_range_ck->checkState() == Qt::Checked)
      //readQImage(_store, n_idx, *_qimg, _curr_idx, _sp_min->value(), _sp_max->value());
      readQImage(_store, n_idx, *_qimg, _curr_flags, _sp_min->value(), _sp_max->value());
      //readQImage(_store, _show_idx, *_qimg, _curr_idx, _sp_min->value(), _sp_max->value());
    else
      //readQImage(_store, n_idx, *_qimg, _curr_idx);
      readQImage(_store, n_idx, *_qimg, _curr_flags);
      //readQImage(_store, _show_idx, *_qimg, _curr_idx);
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
