#ifndef CLIFVIEW_H
#define CLIFVIEW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QTreeWidgetItem>

#include "clif_qt.hpp"

class DatasetRoot;

namespace Ui {
class ClifView;
}

using namespace clif;

class ClifView : public QMainWindow
{
    Q_OBJECT

public:
    explicit ClifView(QWidget *parent = 0);
    ~ClifView();

    void setView(DatasetRoot *root, int idx);

private slots:
    void on_actionOpen_triggered();

    void on_datasetSlider_valueChanged(int value);

    void on_selViewProc_currentIndexChanged(int index);

    void on_tree_itemExpanded(QTreeWidgetItem *item);

    void on_tree_itemActivated(QTreeWidgetItem *item, int column);

    void on_actionSet_horopter_triggered();
    
    void slider_changed_delayed();

private:
    Ui::ClifView *ui;
};

#endif // CLIFVIEW_H
