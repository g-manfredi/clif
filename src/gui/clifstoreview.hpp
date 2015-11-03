#ifndef _CLIFSTOREVIEW_H
#define _CLIFSTOREVIEW_H

#include <QWidget>

#include "config.h"

class QSlider;
class QVBoxLayout;
class QTimer;

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
  
private slots:
  void load_img();
  void queue_sel_img(int n);
  
private:
  QVBoxLayout *_vbox;
  clifScaledImageView *_view;
  QSlider *_slider;
  QImage *_qimg = NULL;
  Datastore *_store = NULL;
  int _curr_idx = -1;
  int _show_idx = 0;
  QTimer *_timer = NULL;
  bool _rendering = false;
  
};
  
}


#endif // SCALEDQGRAPHICSVIEW

