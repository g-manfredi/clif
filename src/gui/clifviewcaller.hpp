#ifndef _CLIF_CLFIVIEW_CALLER_H
#define _CLIF_CLFIVIEW_CALLER_H

#include <QObject>
#include <QLocalSocket>
#include "config.h"

class CLIF_EXPORT ExternalClifViewer : public QObject
{
  Q_OBJECT
  
public:
  ExternalClifViewer(const QString file = QString(), const QString dataset = QString(), QString store = QString());
  
private:
  QLocalSocket *_socket = NULL;
  QString _file;
  QString _dataset;
  QString _store;
  
private slots:
  void connected();
  void error(QLocalSocket::LocalSocketError e);
};

#endif