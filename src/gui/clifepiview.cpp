#include <QtGui/QtGui>

#include "clifepiview.hpp"

DlgFind::DlgFind(QWidget* parent) : QDialog(parent)
{
    label = new QLabel("Find &what:"); // 2.
    what = new QLineEdit();
    label->setBuddy(what); // 3.

    cbCase = new QCheckBox("Match &case");
    cbBack = new QCheckBox("Search &backward");

    btnFind = new QPushButton("&Find"); // 4.
    btnFind->setDefault(true);
    btnFind->setEnabled(false);

    btnClose = new QPushButton("Close");

    // 5.
    connect(what, SIGNAL(textChanged(const QString&)), this, SLOT(enableBtnFind(const QString&)));
    connect(btnFind, SIGNAL(clicked()), this, SLOT(findClicked()));
    connect(btnClose, SIGNAL(clicked()), this, SLOT(close()));

    setLayout(createLayout());
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

    QHBoxLayout* main = new QHBoxLayout();
    main->addLayout(left);
    main->addLayout(right);

    return main;
}

void DlgFind::findClicked() // 8.
{
    somestring = what->text();
  
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

QString DlgFind::getString(QWidget *parent)
{
  DlgFind *finder = new DlgFind(parent);
  
  finder->exec();
  
  QString tmp = finder->somestring;
  
  delete finder;
  
  return tmp;
}