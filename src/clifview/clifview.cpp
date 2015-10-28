#include "clifview.h"
#include "ui_clifview.h"

#include "attribute.hpp"

#include <QTimer>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <opencv2/opencv.hpp>

#include "clifepiview.hpp"
#include "clif_qt.hpp"

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

void attachTreeItem(QTreeWidgetItem *w, StringTree<Attribute*,Datastore*> *t)
{
    if (std::get<0>(t->val.second))
        w->setData(1, Qt::DisplayRole, QString(std::get<0>(t->val.second)->toString().c_str()));
    else if (std::get<1>(t->val.second)) {
        std::stringstream stream;
        stream << *std::get<1>(t->val.second);
        w->setData(1, Qt::DisplayRole, stream.str().c_str());
    }

    for(int i=0;i<t->childCount();i++) {
        QTreeWidgetItem *item = new QTreeWidgetItem(w, QStringList(QString(t->childs[i].val.first.c_str())));
        attachTreeItem(item, &t->childs[i]);
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

    void expand(QTreeWidgetItem *item)
    {
        openDataset();

        if (!expanded) {
            StringTree<Attribute*,Datastore*> tree = dataset->getTree();

            attachTreeItem(item, &tree);
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

ClifView::ClifView(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ClifView)
{
    ui->setupUi(this);

    QStringList headerLabels = {QString("path"), QString("value")};
    ui->tree->setHeaderLabels(headerLabels);
}

ClifView::~ClifView()
{
    delete ui;
}

void ClifView::on_actionOpen_triggered()
{
  QString filename = QFileDialog::getOpenFileName(this,
        tr("Open clif File"));
  
  std::string path(filename.toLocal8Bit().constData());
  lf_file.open(path, H5F_ACC_RDONLY);
  
  //lffile = H5::H5File(filename.toLocal8Bit().constData(), H5F_ACC_RDONLY);

  vector<string> datasets = lf_file.datasetList();
  
  //FIXME clean root list!

  for(uint i=0;i<datasets.size();i++) {
     QString path(datasets[i].c_str());
     DatasetRoot *root = new DatasetRoot(&lf_file, datasets[i]);
     
     QTreeWidgetItem *item = new QTreeWidgetItem(ui->tree, QStringList(path));
     item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
     
     item->setData(0, Qt::UserRole, QVP<DatasetRoot>::asQVariant(root));
     root_list.push_back(root);
  }
}

void ClifView::setView(DatasetRoot *root, int idx)
{
    if (!root)
      return;
    int flags = 0;
    switch (ui->selViewProc->currentIndex()) {
        case 1 : flags = DEMOSAIC; break;
        case 2 : flags = UNDISTORT; break;
    }

    readQImage(root->dataset, idx, curview_q, flags);

    ui->viewer->setImage(curview_q);
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
    setView(root_curr, ui->datasetSlider->value());
}

void ClifView::on_tree_itemExpanded(QTreeWidgetItem *item)
{
    if (!item->data(0, Qt::UserRole).isValid())
      return;
    
    DatasetRoot *root = QVP<DatasetRoot>::asPtr(item->data(0, Qt::UserRole));
    
    root->expand(item);
}

void ClifView::on_tree_itemActivated(QTreeWidgetItem *item, int column)
{
    if (!item->data(0, Qt::UserRole).isValid())
      return;
    
    DatasetRoot *root = QVP<DatasetRoot>::asPtr(item->data(0, Qt::UserRole));
    root_curr = root;
    
    root->openDataset();
    ui->menuTools->actions().at(0)->setEnabled(true);
    
    ui->datasetSlider->setMaximum(root->dataset->Datastore::imgCount()-1);
    ui->datasetSlider->setValue(0);
    
    on_datasetSlider_valueChanged(0);
}

void ClifView::on_actionSet_horopter_triggered()
{
    double h = DlgFind::getDisparity(root_curr->dataset, this);
}
