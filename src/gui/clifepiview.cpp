#include <QtGui/QtGui>

#include <QApplication>
#include <QSplitter>
#include <QGraphicsLineItem>
#include <QTimer>
#include <QSlider>
#include <QVBoxLayout>

#include "dataset.hpp"

#include "clifepiview.hpp"
#include "clif_qt.hpp"

#include "subset3d.hpp"
#include "preproc.hpp"

namespace clif {

clifEpiView::clifEpiView(Dataset *dataset, QWidget* parent)
: QDialog(parent)
{
    _3dslice = new Subset3d(dataset);
    _centerview = new clifScaledImageView(this);
    _epiview = new clifScaledImageView(this);
    _slider = new QSlider(this);
    _slider->setOrientation(Qt::Horizontal);
    //_slider->setStepAlignment(false);
    //_slider->setScaleEngine(new QwtLogScaleEngine());
    //_slider->setScalePosition(QwtSlider::ScalePosition::TrailingScale);
    
    
    /*connect(what, SIGNAL(textChanged(const QString&)), this, SLOT(enableBtnFind(const QString&)));
    connect(btnFind, SIGNAL(clicked()), this, SLOT(findClicked()));*/
    connect(_slider, SIGNAL(valueChanged(int)), this, SLOT(dispChanged(int)));
    connect(_centerview, SIGNAL(imgClicked(QPointF*)), this, SLOT(lineChanged(QPointF*)));

    setLayout(createLayout());
    
    if (!_center_img)
      _center_img = new QImage();
    //FIXME implement via subset3d - we don't actually know what subset3d reads!
    Datastore *store = dataset->getStore(dataset->getSubGroup("calibration/extrinsics")/"data");
    std::vector<int> idx(store->dims(),0);
    readQImage(store, idx, *_center_img, Improc::DEMOSAIC);
    _line = _center_img->size().height()/2;
    _centerview->setImage(*_center_img);
    _line_item = _centerview->scene.addLine(0, _line, _center_img->size().width(),_line);
    
    
    _disp = 0;
    //_slider->blockSignals(true);
    //_slider->setScale(1,1e6);
    //_slider->blockSignals(false);
    _slider->setMaximum(10000);
    _slider->setValue(_disp*100);
    
    setWindowTitle("Find");
    //setFixedHeight(sizeHint().height()); // 6.
}

clifEpiView::~clifEpiView()
{
  if (_center_img)
    delete _center_img;
  if (_epi_img)
    delete _epi_img;
}

/*
QHBoxLayout* clifEpiView::createLayout() // 7.
{
    QHBoxLayout* topLeft = new QHBoxLayout();
    topLeft->addWidget(label);
    topLeft->addWidget(what);

    QVBoxLayout* left = new QVBoxLayout();
    left->addLayout(topLeft);
    left->addWidget(cbCase);
    left->addWidget(cbBack);

    QVBoxLayout* right = new QVBoxLayout();
    right->addWidget(btnFind);
    right->addWidget(btnClose);
    right->addStretch();
    
    right->addWidget(_centerview);
    right->addWidget(_epiview);

    QHBoxLayout* main = new QHBoxLayout();
    main->addLayout(left);
    main->addLayout(right);

    return main;
}*/

QLayout* clifEpiView::createLayout()
{
  QSplitter *splitter = new QSplitter();
  splitter->addWidget(_centerview);
  splitter->addWidget(_epiview);
  
  splitter->setOrientation(Qt::Vertical);
  
  QVBoxLayout* box = new QVBoxLayout();
  box->addWidget(splitter);
  box->addWidget(_slider);
  
  return box;
}
/*
void clifEpiView::findClicked() // 8.
{  
    QString text = what->text();
    Qt::CaseSensitivity cs = cbCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    if(cbBack->isChecked())
    {
        emit findPrev(text, cs);
    }
    else
    {
        emit findNext(text, cs);
    }
}*/
/*
void clifEpiView::enableBtnFind(const QString& text) // 9.
{
    btnFind->setEnabled(!text.isEmpty());
}*/

void clifEpiView::refreshEPI()
{
  //_slider->blockSignals(true);
  if (!_epi_img)
    _epi_img = new QImage();
  readEPI(_3dslice, *_epi_img, _line, _disp);
  _epiview->setImage(*_epi_img);
  qApp->processEvents();
  //_slider->blockSignals(false);
  
  printf("disp %f\n", _disp);
  printf("depth %f\n", _3dslice->disparity2depth(_disp));
}

void clifEpiView::refreshEPISlot()
{
  if (_timer) {
    delete _timer;
    _timer = NULL;
  }
  
  refreshEPI();
}

void clifEpiView::dispChanged(int value)
{
  _disp = value*0.01;
  
  if (!_timer) {
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), SLOT(refreshEPISlot()));
    _timer->setSingleShot(true);
    _timer->start(0);
  }
}

void clifEpiView::lineChanged(QPointF *p)
{
  _line = p->y();
  _epiview->centerOn(p->x(), 0);
  _epiview->setDragMode(QGraphicsView::ScrollHandDrag);
  _line_item->setLine(0, _line,  _center_img->size().width(),_line);
  refreshEPI();
}

double clifEpiView::getDisparity(Dataset *dataset, QWidget *parent)
{
  clifEpiView *finder = new clifEpiView(dataset, parent);
  
  finder->exec();
  
  double d = finder->_disp;
  
  delete finder;
  
  return d;
}

}