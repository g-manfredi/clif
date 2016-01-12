#ifndef _CLIFSTOREVIEW_H
#define _CLIFSTOREVIEW_H

#include <QWidget>
#include <QComboBox>

#include "config.h"

class QSlider;
class QVBoxLayout;
class QTimer;
class QComboBox;
class QCheckBox;
class QDoubleSpinBox;

namespace clif {
/*
class values : public QObject {
 
    Q_OBJECT

  public:
    values(int size) {
      _values = new std::vector<int>(size, 0);

    }
    
    std::vector<int> * getValues() {
      return _values;
    }
    int * getValue(int index) {
      return &(*_values)[index];
    }
  public slots:
    void triggerValue(int index) {
      returnValue(values->[index]);
    }
    void setCurrent(int index) {
      current = index;
    }

    void set(int value) {
      if (current != -2) {
        setCurrentValue(current, value);
      }
      current = -2;

    }
    void setCurrentValue (int index, int value) {
      (*_values)[index] = value;
    }

    signals:
      void returnValue(int value);
  private:
    std::vector<int> * _values;
    int current = -2;

};
*/
class QSlider_adv : public QSlider {

    Q_OBJECT

  public:
    QSlider_adv(Qt::Orientation orientation, QWidget * parent = 0) {
      QSlider(orientation, parent);
    }
    void setDim(int value) {
      dim = value;
    }

  public slots:
    void setDimSlot(int value) {
      dim = value;
    } 
    //void () {
  
  signals:
    //void getDim(int dimension);
    //slMoved(int pos);
 
  private:
    int dim;

};

class list_ext : public QObject {

    Q_OBJECT

  public:
    list_ext(QList<QString> l, int a, int b, int c) {
      list = l;
      backup = l;
      for (int i = 0; i<l.size(); i++) {
        indices.append(i);
      }
      indices_backup = indices;
      gather(a, b, c);
    }
    QStringList getList() {
      return list_final;
    }
      
  public slots:
    void load_list(QList<QString> l) {
      this->list = l;
    }
    void reload_list() {
      list = backup;
    }
    void change1(int index) {
      a = index;
      gather(a,b,c);
      if (a == b) { b = indices[0];}
      if (a == c) { c = indices[1];}
      gather(a,b,c);
      emit list_changed(this->list_final);
      emit update_2(b);
      emit update_3(c);
    }
    void change2(int index) {
      b = index;
      gather(a,b,c);
      if (b == a) { a = indices[0];}
      if (b == c) { c = indices[1];}
      gather(a,b,c);
      emit list_changed(this->list_final);
      emit update_1(a);
      emit update_3(c);

    }
    void change3(int index) {
      c = index;
      gather(a,b,c);
      if (c == a) { a = indices[0];}
      if (c == b) { b = indices[1];}
      gather(a,b,c);
      emit list_changed(this->list_final);
      emit update_1(a);
      emit update_2(b);
      
    }
    void gather(int a, int b, int c) {
      reload_list();
      remove_indices(a, b, c);
      this->a = a,
      this->b = b;
      this->c = c;
      list_final = QStringList(list);      
    }
    void remove_indices(int a, int b, int c) {
      int indices_to_remove[3] = {a,b,c};
      std::sort(indices_to_remove, indices_to_remove+3);
      list.removeAt(indices_to_remove[2]);
      list.removeAt(indices_to_remove[1]);
      list.removeAt(indices_to_remove[0]);
      indices = indices_backup;
      indices.removeAt(a);
      indices.removeAt(b);
      indices.removeAt(c);
    }
  signals:
    void list_changed(QStringList new_list);
    void update_1(int a);
    void update_2(int b);
    void update_3(int c); 

  private:
    QList<QString> list;
    QList<QString> backup;
    QStringList list_final;
    int a, b, c;
    QList<int> indices; 
    QList<int> indices_backup;
    
};

class editable_ComboBox : public QComboBox {

  Q_OBJECT

  public:
    void setID(int id) {
      ID = id;
    }
    void showPopup() {
    }
  public slots:
    void addItems_slot(QStringList list) {
      addItems(list);    
      setCurrentIndex(ID);  
    } 
  private:
    int ID;
};

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
  int _dims;
  
};
  
}

#endif // SCALEDQGRAPHICSVIEW

