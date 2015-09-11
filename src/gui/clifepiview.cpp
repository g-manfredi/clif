#include <QtGui/QtGui>

#include <QApplication>
#include <QSplitter>
#include <QGraphicsLineItem>
#include <QTimer>

#include <qwt_scale_engine.h>

#include "clifepiview.hpp"
#include "clif_qt.hpp"

#include "subset3d.hpp"

namespace clif {

DlgFind::DlgFind(Dataset *dataset, QWidget* parent)
: QDialog(parent)
{
    _3dslice = new Subset3d(dataset);
    _centerview = new clifScaledImageView(this);
    _epiview = new clifScaledImageView(this);
    _slider = new QwtSlider(this);
    _slider->setOrientation(Qt::Horizontal);
    _slider->setStepAlignment(false);
    _slider->setScaleEngine(new QwtLogScaleEngine());
    _slider->setScalePosition(QwtSlider::ScalePosition::TrailingScale);
    
    
    /*connect(what, SIGNAL(textChanged(const QString&)), this, SLOT(enableBtnFind(const QString&)));
    connect(btnFind, SIGNAL(clicked()), this, SLOT(findClicked()));*/
    connect(_slider, SIGNAL(valueChanged(double)), this, SLOT(horopterChanged(double)));
    connect(_centerview, SIGNAL(imgClicked(QPointF*)), this, SLOT(lineChanged(QPointF*)));

    setLayout(createLayout());
    
    readQImage(dataset, 0, _center_img, DEMOSAIC);
    _line = _center_img.size().height()/2;
    _centerview->setImage(_center_img);
    _line_item = _centerview->scene.addLine(0, _line,  _center_img.size().width(),_line);
    
    
    _depth = 1000;
    _slider->blockSignals(true);
    _slider->setScale(1,1e6);
    _slider->blockSignals(false);
    _slider->setValue(_depth);
    
    setWindowTitle("Find");
    //setFixedHeight(sizeHint().height()); // 6.
}
/*
QHBoxLayout* DlgFind::createLayout() // 7.
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

QVBoxLayout* DlgFind::createLayout()
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
void DlgFind::findClicked() // 8.
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
void DlgFind::enableBtnFind(const QString& text) // 9.
{
    btnFind->setEnabled(!text.isEmpty());
}*/

void DlgFind::refreshEPI()
{
  //_slider->blockSignals(true);
  readEPI(_3dslice, _epi_img, _line, _depth);
  _epiview->setImage(_epi_img);
  qApp->processEvents();
  //_slider->blockSignals(false);
}

void DlgFind::refreshEPISlot()
{
  if (_timer) {
    delete _timer;
    _timer = NULL;
  }
  
  refreshEPI();
}

void DlgFind::horopterChanged(double value)
{
  _depth = value;
  
  if (!_timer) {
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), SLOT(refreshEPISlot()));
    _timer->setSingleShot(true);
    _timer->start(0);
  }
  
  //refreshEPI();
}

void DlgFind::lineChanged(QPointF *p)
{
  _line = p->y();
  _epiview->centerOn(p->x(), 0);
  _epiview->setDragMode(QGraphicsView::ScrollHandDrag);
  _line_item->setLine(0, _line,  _center_img.size().width(),_line);
  refreshEPI();
}

double DlgFind::getHoropterDepth(Dataset *dataset, QWidget *parent)
{
  DlgFind *finder = new DlgFind(dataset, parent);
  
  finder->exec();
  
  double h = finder->_depth;
  
  delete finder;
  
  return h;
}

}