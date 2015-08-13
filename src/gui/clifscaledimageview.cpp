#include "clifscaledimageview.hpp"

#include <QMouseEvent>
#include <QPixmap>

namespace clif_qt {

clifScaledImageView::clifScaledImageView(QWidget *parent)
    : QGraphicsView(parent)
{
        setScene(&scene);
        setDragMode(QGraphicsView::ScrollHandDrag);
};


void clifScaledImageView::resizeEvent(QResizeEvent * event)
{
    if (fit)
        fitInView(sceneRect(), Qt::KeepAspectRatio);

    QGraphicsView::resizeEvent(event);
}

void clifScaledImageView::mousePressEvent(QMouseEvent *me)
{
 if(me->button()==Qt::MiddleButton) {
    fit = !fit;

    if (fit)
        fitInView(sceneRect(), Qt::KeepAspectRatio);
    else {
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setTransform(QTransform::fromScale(1,1));
    }
  }
 else {
    QPointF p = mapToScene(me->pos());
    emit imgClicked(&p);
    QGraphicsView::mousePressEvent(me);
 }
}

void clifScaledImageView::wheelEvent(QWheelEvent * event)
{
    fit = false;

    //FIXME strange unit? No fixed "step" possible?
    int steps = event->angleDelta().y() / 120;

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    scale(pow(2, steps),pow(2, steps));

    event->accept();
}

void clifScaledImageView::setImage(QImage &img)
{
    scene.clear();

    scene.addPixmap(QPixmap::fromImage(img));
    
    show();
}

}