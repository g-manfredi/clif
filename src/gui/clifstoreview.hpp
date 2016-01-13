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

class IndicesHandler : public QObject { 
	
    Q_OBJECT

  public:
    IndicesHandler(int size) {
      data.fill(0,size);
    }
    QVector<int> * getData() {
      return &data;
    }
    int getValue(int index) {
      return data.at(index);
    }

    
  public slots: 
    void changeEntry(int value) {
      data.insert(sender()->property("Dimension").toInt(), value);
    }
  private:
    QVector<int> data;
};    

class Slider_dim : public QSlider {

    Q_OBJECT
    Q_PROPERTY(int Dimension READ getDim WRITE setDim)

  public:
    Slider_dim(Qt::Orientation orientation, QWidget *parent = NULL)
    : QSlider(orientation, parent) { }
    
    
    void setDim(int index) {
      _dimension = index;

    }
    int getDim() {
      return _dimension;
    }

  public slots:
    void update(int index) {
      setProperty("Dimension",index);
      setValue(handler->getValue(index));
    } 
 
  private:
    int _dimension;
    QComboBox * box;
    IndicesHandler * handler;

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
    QList<int> getIndices() {
      return indices;
    }
  public slots:
    void load_list(QList<QString> l) {
      this->list = l;
    }
    void reload_list() {
      list = backup;
    }
    void change1(int index) {//TODO
      a = index;
      gather(a,b,c);
      if (a == b) { b = indices[0];}
      if (a == c) { c = indices[0];}
      if (b == c) { c = indices[1];}
      gather(a,b,c);
      emit list_changed(this->list_final);
      emit update_2(b);
      emit update_3(c);
    }
    void change2(int index) {//TODO
      b = index;
      gather(a,b,c);
      if (b == a) { a = indices[0];}
      if (b == c) { c = indices[0];}
      if (a == c) { c = indices[1];}
      gather(a,b,c);
      emit list_changed(this->list_final);
      emit update_1(a);
      emit update_3(c);

    }
    void change3(int index) {//TODO
      c = index;
      gather(a,b,c);
      if (c == a) { a = indices[0];}
      if (c == b) { b = indices[0];}
      if (a == b) { b = indices[1];}
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
    list_ext * Database;
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

