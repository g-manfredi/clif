#ifndef _CLIFEPIDIALOG
#define _CLIFEPIDIALOG

#include <clif/clif.hpp>
#include <clif/subset3d.hpp>

#include "clifscaledimageview.hpp"

#include <QDialog>

#include <clif/config.hpp>

class QVBoxLayout;
class QImage;
class QSlider;

namespace clif {
  
class CLIF_EXPORT clifEpiView : public QDialog
{
    Q_OBJECT

public:
    clifEpiView(clif::Dataset *dataset, QWidget* parent = 0);
    ~clifEpiView();
    
    static double getDisparity(clif::Dataset *dataset, QWidget *parent = 0);

signals:
    void findNext(const QString& str, Qt::CaseSensitivity cs);
    void findPrev(const QString& str, Qt::CaseSensitivity cs);

private slots:
    //void findClicked();
    //void enableBtnFind(const QString& text);
  
    void dispChanged(int value);
    void lineChanged(QPointF *p);
    void refreshEPISlot();

private:
    QLayout* createLayout();
    void refreshEPI();

    QSlider *_slider;
    clifScaledImageView *_centerview;
    clifScaledImageView *_epiview;
    
    QImage *_center_img = NULL;
    QImage *_epi_img = NULL;
    
    double _disp;
    int _line;
    clif::Subset3d *_3dslice;
    
    QTimer *_timer = NULL;
    
    QGraphicsLineItem *_line_item;
};

}
#endif

