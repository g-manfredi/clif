#ifndef _SCALEDQGRAPHICSVIEW_H
#define _SCALEDQGRAPHICSVIEW_H

#include <QtGui>
#include <QGraphicsView>

#if defined WTF_HACK_QT_EXPORT
 #define TEST_COMMON_DLLSPEC Q_DECL_EXPORT
#else
 #define TEST_COMMON_DLLSPEC Q_DECL_IMPORT
#endif

namespace clif {
  
class TEST_COMMON_DLLSPEC clifScaledImageView : public QGraphicsView
{
    Q_OBJECT

public:
    clifScaledImageView(QWidget *parent = 0);
    
    void setImage(QImage &img);
    
    QGraphicsScene scene;
    
signals:
    void imgClicked(QPointF *p);
 
protected:
    void resizeEvent(QResizeEvent * event);
    void mousePressEvent(QMouseEvent *me);
    void wheelEvent(QWheelEvent * event);
private:
    bool fit = true;
};

}


#endif // SCALEDQGRAPHICSVIEW

