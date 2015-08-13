#include "clifscaledgraphicsview.hpp"

#include <QMouseEvent>
#include <QPixmap>


clifScaledGraphicsView::clifScaledGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
        setScene(&scene);
};


void clifScaledGraphicsView::resizeEvent(QResizeEvent * event)
{
    if (fit)
        fitInView(sceneRect(), Qt::KeepAspectRatio);

    QGraphicsView::resizeEvent(event);
}

void clifScaledGraphicsView::mousePressEvent(QMouseEvent *me)
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
 else
    QGraphicsView::mousePressEvent(me);
}

void clifScaledGraphicsView::wheelEvent(QWheelEvent * event)
{
    fit = false;

    //FIXME strange unit? No fixed "step" possible?
    int steps = event->angleDelta().y() / 120;

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    scale(pow(2, steps),pow(2, steps));

    event->accept();
}

void clifScaledGraphicsView::setImage(QImage &img)
{
    scene.clear();

    scene.addPixmap(QPixmap::fromImage(img));
    
    show();
}
