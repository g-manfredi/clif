#include "clifview.h"
#include "ui_clifview.h"

#include <QTimer>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QtNetwork>

#include <opencv2/opencv.hpp>

#include "clifepiview.hpp"
#include "clif_qt.hpp"
#include "dataset.hpp"
#include "clifstoreview.hpp"
#include "clifviewcaller.hpp"

using namespace clif;
using namespace std;
using namespace cv;

//H5::H5File lffile;
//Mat curview;
QImage curview_q;
QTimer *_timer = NULL;

ClifFile lf_file;

vector<DatasetRoot*> root_list;
DatasetRoot *root_curr = NULL;

clifStoreView *_storeview = NULL;

void attachTreeItem(QTreeWidgetItem *w, StringTree<Attribute*,Datastore*> *t, ClifView *view = NULL, const char *select_store = NULL)
{
    if (std::get<0>(t->val.second)) {
        w->setData(1, Qt::DisplayRole, QString(std::get<0>(t->val.second)->toString().c_str()));
    }
    else if (std::get<1>(t->val.second)) {
        std::stringstream stream;
        stream << *std::get<1>(t->val.second);
        w->setData(1, Qt::DisplayRole, stream.str().c_str());
        w->setData(1, Qt::UserRole, qVariantFromValue((void*)std::get<1>(t->val.second)));
        if (select_store && !std::get<1>(t->val.second)->getDatastorePath().compare(select_store)) {
          w->setSelected(true);
          view->on_tree_itemActivated(w, 0);
        }
    }

    for(int i=0;i<t->childCount();i++) {
        QTreeWidgetItem *item = new QTreeWidgetItem(w, QStringList(QString(t->childs[i].val.first.c_str())));
        attachTreeItem(item, &t->childs[i], view, select_store);
    }
}

class DatasetRoot {
public:
    bool expanded = false;
    Dataset *dataset = NULL;
    ClifFile *f = NULL;
    std::string name;
    int cur_view = 0;

    DatasetRoot(ClifFile *f_, std::string name_) : f(f_), name(name_) {};

    void openDataset()
    {
        if (!dataset)
          dataset = f->openDataset(name);
    }

    void expand(QTreeWidgetItem *item, ClifView *view = NULL, const char *expand_store = NULL)
    {
        openDataset();

        if (!expanded) {
            StringTree<Attribute*,Datastore*> tree = dataset->getTree();

            attachTreeItem(item, &tree, view, expand_store);
        }
    }
    
    ~DatasetRoot()
    {
      if (dataset)
        delete dataset;
    }
};

template <class T> class QVP
{  
public:
    static T* asPtr(QVariant v) { return  (T *) v.value<void *>(); }
    static QVariant asQVariant(T* ptr) { return qVariantFromValue((void *) ptr); }
};
/*
QImage  cvMatToQImage( const cv::Mat &inMat )
{
  switch ( inMat.type() )
  {
     // 8-bit, 4 channel
     case CV_8UC4:
     {
        QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_RGB32 );

        return image;
     }

     // 8-bit, 3 channel
     case CV_8UC3:
     {
        QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_RGB888 );

        return image.rgbSwapped();
     }

     // 8-bit, 1 channel
     case CV_8UC1:
     {
        static QVector<QRgb>  sColorTable;

        // only create our color table once
        if ( sColorTable.isEmpty() )
        {
           for ( int i = 0; i < 256; ++i )
              sColorTable.push_back( qRgb( i, i, i ) );
        }

        QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_Indexed8 );

        image.setColorTable( sColorTable );

        return image;
     }

     default:
        printf("conversion error!\n");
        //std::cout << "::cvMatToQImage() - cv::Mat image type not handled in switch:" << inMat.type();
        break;
  }

  return QImage();
}*/

static QLocalServer* add_server(ClifView *clifview)
{
  QLocalServer *server = new QLocalServer(clifview);
  if (!server->listen("clifview_showfile")) {
    printf("could not start server - will not listen to show requests - %s!\n", server->errorString().toUtf8().constData());
    delete server;
    return NULL;
  }
  
  ClifView::connect(server, SIGNAL(newConnection()), clifview, SLOT(showFileNewCon()));
  
  return server;
}

ClifView::ClifView(const char *cliffile, const char *dataset,  const char *store, bool del, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ClifView)
{    
    ui->setupUi(this);

    QStringList headerLabels = {QString("path"), QString("value")};
    ui->tree->setHeaderLabels(headerLabels);
    
    if (cliffile)
      open(cliffile, dataset, store, del);
    
    _server = add_server(this);
}

void ClifView::showFileNewCon()
{
    _server_socket = _server->nextPendingConnection();
    
    if (!_server_socket || _server_socket->state() != QLocalSocket::ConnectedState) {
      printf("error getting local socket %s\n", _server_socket->errorString().toUtf8().constData());
      if (_server_socket) {
        delete _server_socket;
        _server_socket = NULL;
      }
    }
    else {    
      connect(_server_socket, SIGNAL(readyRead()), this, SLOT(showFileReadyRead()));
    }
}

void ClifView::showFileReadyRead()
{
  QString filename;
  QString dataset;
  QString store;
  bool del = false;
  printf("client connected!\n");
  
  QDataStream in(_server_socket);
  in.setVersion(QDataStream::Qt_4_0);
  
  in >> filename;
  in >> dataset;
  in >> store;
  in >> del;

  char *fs = strdup(filename.toUtf8().constData());
  char *ds = strdup(dataset.toUtf8().constData());
  char *ss = strdup(store.toUtf8().constData());
 
  delete _client_socket;
  
  open(fs, ds, ss, del);

  free(fs);
  free(ds);
  free(ss);
}

/*void ClifView::showFileClientConnected()
{
  int written;
  char buf[4096];
  
  sprintf(buf, "%s\n", "/home/hendrik/projects/clif/demo.clif");
  
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_4_0);
  out << tr("/home/hendrik/projects/clif/examples/demo.clif");
  //out << tr("default");
  //out << tr("data");
  
  _client_socket->write(block);
}

void ClifView::showFileClientError(QLocalSocket::LocalSocketError e)
{
    printf("client error %d - %s!\n", e, _client_socket->errorString().toUtf8().constData());
}*/

ClifView::~ClifView()
{
    delete ui;

#ifdef _WIN32
  for(auto f : _del_on_exit)
    remove(f.c_str());
#endif
}

void ClifView::on_actionOpen_triggered()
{
  QString filename = QFileDialog::getOpenFileName(this,
        tr("Open clif File"));
  
  char *path = strdup(filename.toUtf8().constData());

  open(path);

  free(path);
}


void ClifView::open(const char *cliffile, const char *dataset,  const char *store, bool del)
{  
  lf_file.open(cliffile, H5F_ACC_RDONLY);
  _load_store = NULL;
      
#ifndef _WIN32
    if (del)
      unlink(cliffile);
#else
    _del_on_exit.push_back(cliffile);
#endif
    
  
  vector<string> datasets = lf_file.datasetList();
  
  //FIXME clean root list!

  for(uint i=0;i<datasets.size();i++) {
     QString path(datasets[i].c_str());
     DatasetRoot *root = new DatasetRoot(&lf_file, datasets[i]);
     
     QTreeWidgetItem *item = new QTreeWidgetItem(ui->tree, QStringList(path));
     item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
     
     item->setData(0, Qt::UserRole, QVP<DatasetRoot>::asQVariant(root));
     root_list.push_back(root);
     
     if (dataset && !datasets[i].compare(dataset)) {
       _load_store = store;       
       ui->tree->expandItem(item);
     }
  }
}

//FIXME multi-dim!
void ClifView::setView(DatasetRoot *root, int idx)
{
    /*if (!root)
      return;
    int flags = 0;
    switch (ui->selViewProc->currentIndex()) {
        case 1 : flags = DEMOSAIC; break;
        case 2 : flags = UNDISTORT; break;
    }

    std::vector<int> n_idx(root->dataset->dims(),0);
    n_idx[3] = idx;
    
    readQImage(root->dataset, n_idx, curview_q, flags);
*/
}

void ClifView::slider_changed_delayed()
{
  if (_timer) {
    delete _timer;
    _timer = NULL;
  }
  
  setView(root_curr, root_curr->cur_view);
  qApp->processEvents();
}

void ClifView::on_datasetSlider_valueChanged(int value)
{
  if (!_timer) {
    root_curr->cur_view = value;
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), SLOT(slider_changed_delayed()));
    _timer->setSingleShot(true);
    _timer->start(0);
  }
}

void ClifView::on_selViewProc_currentIndexChanged(int index)
{
    //setView(root_curr, ui->datasetSlider->value());
}

void ClifView::on_tree_itemExpanded(QTreeWidgetItem *item)
{
    if (!item->data(0, Qt::UserRole).isValid())
      return;
    
    DatasetRoot *root = QVP<DatasetRoot>::asPtr(item->data(0, Qt::UserRole));
    
    root->expand(item, this, _load_store);
}

void ClifView::on_tree_itemActivated(QTreeWidgetItem *item, int column)
{
    if (item->data(0, Qt::UserRole).isValid()) {
      DatasetRoot *root = QVP<DatasetRoot>::asPtr(item->data(0, Qt::UserRole));
      root_curr = root;
      
      root->openDataset();
      ui->menuTools->actions().at(0)->setEnabled(true);
    }
    
    if (item->data(1, Qt::UserRole).isValid() && item->data(1, Qt::UserRole).value<void*>()) {
      if (_storeview)
        delete _storeview;
      
      _storeview = new clifStoreView((Datastore*)item->data(1, Qt::UserRole).value<void*>());
      
      ui->viewDock->setWidget(_storeview);
    }
    
    //ui->datasetSlider->setMaximum(root->dataset->Datastore::imgCount()-1);
    //ui->datasetSlider->setValue(0);
    
    //on_datasetSlider_valueChanged(0);
}

void ClifView::on_actionSet_horopter_triggered()
{
    double h = clifEpiView::getDisparity(root_curr->dataset, this);
}
