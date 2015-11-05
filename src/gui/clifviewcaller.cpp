#include "clifviewcaller.hpp"

#include <QDataStream>
#include <QProcess>

ExternalClifViewer::ExternalClifViewer(QString file, QString dataset, QString store)
{
  _socket = new QLocalSocket(this);
  
  _file = file;
  _dataset = dataset;
  _store = store;
  
  connect(_socket, SIGNAL(connected()), this, SLOT(connected()));
  connect(_socket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(error(QLocalSocket::LocalSocketError)));
  
  _socket->connectToServer("clifview_showfile", QIODevice::ReadWrite);
}

void ExternalClifViewer::connected()
{
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_4_0);
  out << _file;
  out << _dataset;
  out << _store;
  
  _socket->write(block);
  
  _file = QString();
  
  _socket->disconnectFromServer();
}

void ExternalClifViewer::error(QLocalSocket::LocalSocketError e)
{
  if (!_file.size())
    return;
  
  printf("could not find/connect running clifview %s!\n", _socket->errorString().toUtf8().constData());
  printf("starting new instance\n");
  
  QProcess *process = new QProcess();
  QStringList args = {"-i", _file};
  if (_dataset.size())
    args << "-d" << _dataset;
  if (_store.size())
    args << "-s" << _store;
  
  QString file = "/home/hendrik/projects/clif/build/src/clifview/clifview";
  process->start(file, args);
}