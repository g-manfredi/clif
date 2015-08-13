#include <QtGui/QtGui>

#include "clifepiview.hpp"
#include "clifqt.hpp"

namespace clif_qt {

DlgFind::DlgFind(ClifDataset *dataset, QWidget* parent)
: QDialog(parent), _dataset(dataset)
{
    label = new QLabel("Find &what:"); // 2.
    what = new QLineEdit();
    label->setBuddy(what); // 3.

    cbCase = new QCheckBox("Match &case");
    cbBack = new QCheckBox("Search &backward");

    _centerview = new clifScaledImageView(this);
    _epiview = new clifScaledImageView(this);
    
    btnFind = new QPushButton("&Find"); // 4.
    btnFind->setDefault(true);
    btnFind->setEnabled(false);

    btnClose = new QPushButton("Close");

    // 5.
    connect(what, SIGNAL(textChanged(const QString&)), this, SLOT(enableBtnFind(const QString&)));
    connect(btnFind, SIGNAL(clicked()), this, SLOT(findClicked()));
    connect(btnClose, SIGNAL(clicked()), this, SLOT(close()));

    setLayout(createLayout());
    
    readQImage(*dataset, 0, _center_img, CLIF_DEMOSAIC);
    _centerview->setImage(_center_img);
    
    readEPI(*dataset, _epi_img, 200);
    _epiview->setImage(_epi_img);  
    
    setWindowTitle("Find");
    setFixedHeight(sizeHint().height()); // 6.
}

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
}

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
}

void DlgFind::enableBtnFind(const QString& text) // 9.
{
    btnFind->setEnabled(!text.isEmpty());
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