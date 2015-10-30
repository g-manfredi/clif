#include "clifstoreview.hpp"

#include <QtGui/QtGui>

#include <QApplication>
#include <QSplitter>
#include <QGraphicsLineItem>
#include <QTimer>
#include <QSlider>
#include <QVBoxLayout>

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
  
  _view = new clifScaledImageView(_vbox);
  v_box->addWidget(_view);
  
}

}