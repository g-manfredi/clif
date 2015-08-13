#include <QtGui/QtGui>

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

    setLayout(createLayout());
    
    readQImage(*dataset, 0, _center_img, CLIF_DEMOSAIC);
    _centerview->setImage(_center_img);
    
    readEPI(*dataset, _epi_img, 200, 0);
    _epiview->setImage(_epi_img);
    
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
  _horopter = value;
  readEPI(*_dataset, _epi_img, 200, _horopter);
  _epiview->setImage(_epi_img);
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