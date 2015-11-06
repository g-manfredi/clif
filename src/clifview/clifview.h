#ifndef CLIFVIEW_H
#define CLIFVIEW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QTreeWidgetItem>

#include <QLocalSocket>

#include "clif_qt.hpp"

class DatasetRoot;
class QLocalServer;

namespace Ui {
class ClifView;
}

using namespace clif;

class ClifView : public QMainWindow
{
    Q_OBJECT

public:
    explicit ClifView(const char *cliffile = NULL, const char *dataset = NULL,  const char *store = NULL, bool del = false, QWidget *parent = 0);
    ~ClifView();

    void setView(DatasetRoot *root, int idx);
    void open(const char *cliffile = NULL, const char *dataset = NULL,  const char *store = NULL, bool del = false);

public slots:
    
    void on_tree_itemActivated(QTreeWidgetItem *item, int column);
    
private slots:
    void on_actionOpen_triggered();
    void on_datasetSlider_valueChanged(int value);
    void on_selViewProc_currentIndexChanged(int index);
    void on_tree_itemExpanded(QTreeWidgetItem *item);
    void on_actionSet_horopter_triggered();
    void slider_changed_delayed();
    void showFileNewCon();
    void showFileReadyRead();
    //void showFileClientConnected();
    //void showFileClientError(QLocalSocket::LocalSocketError e);
    
private:
    Ui::ClifView *ui;
    const char *_load_store = NULL;
    
    QLocalServer *_server = NULL;
    QLocalSocket *_server_socket = NULL;
    QLocalSocket *_client_socket = NULL;
    
    std::vector<std::string> _del_on_exit;
};

#endif // CLIFVIEW_H
