#ifndef _CLIFSCALEDIMAGEVIEW_H
#define _CLIFSCALEDIMAGEVIEW_H

#include <QtGui>
#include <QGraphicsView>
#include "config.h"

namespace clif {
  
class CLIF_EXPORT clifScaledImageView : public QGraphicsView
{
    Q_OBJECT

public:
    clifScaledImageView(QWidget *parent = 0);
    
    void setImage(QImage &img);
    
    QGraphicsScene scene;
    
signals:
    void imgClicked(QPointF *p);
    void dragged(QPointF *p);
 
protected:
    void resizeEvent(QResizeEvent * event);
    void mousePressEvent(QMouseEvent *me);
    void wheelEvent(QWheelEvent * event);
private:
    bool fit = true;
};

}


#endif // SCALEDQGRAPHICSVIEW

