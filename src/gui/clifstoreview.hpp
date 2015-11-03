#ifndef _CLIFSTOREVIEW_H
#define _CLIFSTOREVIEW_H

#include <QWidget>

#include "config.h"

class QSlider;
class QVBoxLayout;

namespace clif {
  
class clifScaledImageView;
class Datastore;
  
class CLIF_EXPORT clifStoreView : public QWidget
{
  Q_OBJECT
  
public:
  clifStoreView();
  clifStoreView(Datastore *store, QWidget* parent = NULL);
  virtual ~clifStoreView();
  
private:
  QVBoxLayout *_vbox;
  clifScaledImageView *_view;
  QSlider *_slider;
  QImage *_qimg = NULL;
};
  
}


#endif // SCALEDQGRAPHICSVIEW

