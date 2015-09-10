#ifndef _CLIFEPIDIALOG
#define _CLIFEPIDIALOG

#include "clif.hpp"

#include "clifscaledimageview.hpp"

#include <QtGui>
#include <QDialog>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <qwt_slider.h>

#if defined WTF_HACK_QT_EXPORT
 #define TEST_COMMON_DLLSPEC Q_DECL_EXPORT
#else
 #define TEST_COMMON_DLLSPEC Q_DECL_IMPORT
#endif

namespace clif_qt {
  
class TEST_COMMON_DLLSPEC DlgFind : public QDialog
{
    Q_OBJECT

public:
    DlgFind(clif::Dataset *dataset, QWidget* parent = 0);
    
    static double getHoropterDepth(clif::Dataset *dataset, QWidget *parent = 0);

signals:
    void findNext(const QString& str, Qt::CaseSensitivity cs);
    void findPrev(const QString& str, Qt::CaseSensitivity cs);

private slots:
    //void findClicked();
    //void enableBtnFind(const QString& text);
  
    void horopterChanged(double value);
    void lineChanged(QPointF *p);
    void refreshEPISlot();

private:
    QVBoxLayout* createLayout();
    void refreshEPI();

    QwtSlider *_slider;
    clifScaledImageView *_centerview;
    clifScaledImageView *_epiview;
    
    QImage _center_img;
    QImage _epi_img;
    
    double _depth;
    int _line;
    Clif3DSubset *_3dslice;
    
    QTimer *_timer = NULL;
    
    QGraphicsLineItem *_line_item;
};

}
#endif

