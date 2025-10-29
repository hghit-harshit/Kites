#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QTextEdit>
#include <QTableWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QWidget>
#include <map>
#include "ui/kitestab.h"
#include "ui/registercontainer.h"
#include "vm/vm_manager.h"
#include <memory>
namespace Kites
{
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum class TabIndex {
        EditorTabIndex = 0,
        MemoryTabIndex,
        ProcessorTabIndex
    };

public:
    MainWindow(QWidget *parent = nullptr);
    void setUpUI();
    //void connectActions();
    ~MainWindow();

private:

    void setUpToolBar();
    void setUpSidebar();
    void setUpMenubar();
    void setUpTabs();
    void run();
    void processorChangeDialog();
    Ui::MainWindow *ui;
    
    //QToolBar *m_mainToolbar = nullptr;
    QListWidget* m_sidebar = nullptr;
    QStackedWidget *m_stackedTabs = nullptr;
    RegisterContainer* m_registerContainer = nullptr;
    std::map<TabIndex, KitesTab*> m_tabs;
    TabIndex m_currentTabIndex = TabIndex::EditorTabIndex;

    VMManager *m_vmManager = nullptr;

};
} // namespace Kites
