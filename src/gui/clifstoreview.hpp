#ifndef _CLIFSTOREVIEW_H
#define _CLIFSTOREVIEW_H

#include <QWidget>

#include <clif/config.hpp>

class QSlider;
class QVBoxLayout;
class QTimer;
class QComboBox;
class QCheckBox;
class QDoubleSpinBox;

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
  void queue_load_img();
  void rangeStateChanged(int s);
  
private:
  clifScaledImageView *_view = NULL;
  QImage *_qimg = NULL;
  Datastore *_store = NULL;
  int _curr_idx = -1;
  int _curr_flags = 0;
  int _show_idx = 0;
  QTimer *_timer = NULL;
  QComboBox *_sel = NULL;
  bool _rendering = false;
  QCheckBox *_range_ck = NULL;
  QCheckBox *_try_out_new_reader = NULL;
  QDoubleSpinBox *_sp_min = NULL;
  QDoubleSpinBox *_sp_max = NULL;
  
  QSlider *_depth_slider = NULL;
};
  
}


#endif // SCALEDQGRAPHICSVIEW

