#include <QtGui/QtGui>

#include <QApplication>

#include <QSplitter>

#include "clifepiview.hpp"
#include "clifqt.hpp"

namespace clif_qt {

DlgFind::DlgFind(ClifDataset *dataset, QWidget* parent)
: QDialog(parent), _dataset(dataset)
{
    _centerview = new clifScaledImageView(this);
    _epiview = new clifScaledImageView(this);
    _slider = new QSlider(this);
    _slider->setOrientation(Qt::Horizontal);
    
    /*connect(what, SIGNAL(textChanged(const QString&)), this, SLOT(enableBtnFind(const QString&)));
    connect(btnFind, SIGNAL(clicked()), this, SLOT(findClicked()));*/
    connect(_slider, SIGNAL(valueChanged(int)), this, SLOT(horopterChanged(int)));
    connect(_centerview, SIGNAL(imgClicked(QPointF*)), this, SLOT(lineChanged(QPointF*)));

    setLayout(createLayout());
    
    readQImage(*dataset, 0, _center_img, CLIF_DEMOSAIC);
    _centerview->setImage(_center_img);
    
    _line = _center_img.size().height()/2;
    horopterChanged(0);
    
    _slider->setMaximum(50);
    _slider->setMinimum(-50);
    _slider->setValue(0);
    
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

void DlgFind::horopterChanged(int value)
{
  _slider->blockSignals(true);
  _horopter = value;
  readEPI(*_dataset, _epi_img, _line, _horopter);
  _epiview->setImage(_epi_img);
  qApp->processEvents();
  _slider->blockSignals(false);
}

void DlgFind::lineChanged(QPointF *p)
{
  _slider->blockSignals(true);
  _line = p->y();
  readEPI(*_dataset, _epi_img, _line, _horopter);
  _epiview->setImage(_epi_img);
  _epiview->centerOn(p->x(), 0);
  qApp->processEvents();
  _slider->blockSignals(false);
}

double DlgFind::getHoropter(ClifDataset *dataset, QWidget *parent)
{
  DlgFind *finder = new DlgFind(dataset, parent);
  
  finder->exec();
  
  double h = finder->_horopter;
  
  delete finder;
  
  return h;
}

}