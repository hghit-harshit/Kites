#pragma once
#include "ui/common/kitestab.h"
#include "ui/register_table/registercontainer.h"
#include "profiler/profiler.h"
#include "vm/vm_manager.h"
#include <QColor>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QPalette> // For setting the colors
#include <QSplitter>
#include <QStackedWidget>
#include <QStyleFactory> // For setting the style
#include <QTextCharFormat>
#include <QThread>
#include <QWidget>
#include <map>
#include <memory>

namespace Kites
{
QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum class TabIndex
    {
        EditorTabIndex = 0,
        MemoryTabIndex,
        ProcessorTabIndex,
        CacheTabIndex,
        CompilerTabIndex,
        ProfilerTabIndex
    };

    enum class Theme
    {
        Light,
        Dark
    }; // for now only two can add more in future

  public:
    MainWindow(QWidget *parent = nullptr);
    void setUpUI();
    // void connectActions();
    ~MainWindow();

  private:
    void setUpStatusBar();
    void setUpToolBar();
    void setUpSidebar();
    void setUpMenubar();
    void setUpTabs();
    void setUpPalettes();
    void disableToolBarButtons();
    void toggleTheme(Theme theme);
    bool tryParseAndLoadProgram();
    void run();
    void processorChangeDialog();
    Ui::MainWindow *ui;

    // QToolBar *m_mainToolbar = nullptr;
    QListWidget       *m_sidebar           = nullptr;
    QStackedWidget    *m_stackedTabs       = nullptr;
    RegisterContainer *m_registerContainer = nullptr;
    VMManager         *m_vmManager         = nullptr;
    QThread           *m_vmThread          = nullptr;
    TabIndex           m_currentTabIndex   = TabIndex::EditorTabIndex;

    std::map<TabIndex, KitesTab *> m_tabs;
    std::map<Theme, QPalette>      m_palettes;
  public slots:
    void vmChanged(const VMType &vmType); // this will catch the signal from processor dialog
    void runFinishedSlot();
    // void runErrorSlot();
  signals:
    void vmChangedSignal();
    void runVMSignal();
};
} // namespace Kites
