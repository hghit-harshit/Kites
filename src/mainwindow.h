#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QMessageBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QTextCharFormat>
#include <QWidget>
#include <map>
#include "ui/kitestab.h"
#include "ui/registercontainer.h"
#include "vm/vm_manager.h"
#include <memory>
#include <QStyleFactory> // For setting the style
#include <QPalette>      // For setting the colors
#include <QColor>
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

    enum class Theme
    {
        Light,
        Dark
    }; // for now only two can add more in future

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
    void setUpPalettes();
    void toggleTheme(Theme theme);
    bool tryParseAndLoadProgram();
    void run();
    void processorChangeDialog();
    Ui::MainWindow *ui;
    
    //QToolBar *m_mainToolbar = nullptr;
    QListWidget* m_sidebar = nullptr;
    QStackedWidget *m_stackedTabs = nullptr;
    RegisterContainer* m_registerContainer = nullptr;
    std::map<TabIndex, KitesTab*> m_tabs;
    TabIndex m_currentTabIndex = TabIndex::EditorTabIndex;
    std::unique_ptr<VMManager> m_vmManager = nullptr;
    std::map<Theme,QPalette> m_palettes;
public slots:
    void vmChanged(const VMType& vmType); // this will catch the signal from processor dialog

signals:
    void vmChangedSignal();
};
} // namespace Kites
