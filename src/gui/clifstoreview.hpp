#ifndef _CLIFSTOREVIEW_H
#define _CLIFSTOREVIEW_H

#include <QWidget>

#include "config.h"

class QVBoxLayout;

namespace clif {
  
class clifScaledImageView;
  
class CLIF_EXPORT clifStoreView : public QWidget
{
  Q_OBJECT
  
public:
  clifStoreView();
  clifStoreView(Datastore *store, QWidget* parent);
  
private:
  QVBoxLayout *_vbox;
  clifScaledImageView *_view;
};
  
}


#endif // SCALEDQGRAPHICSVIEW

