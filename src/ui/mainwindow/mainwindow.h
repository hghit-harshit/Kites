#pragma once
#include "processor/processor_manager.h"
#include "ui/common/kitestab.h"
#include "ui/register_table/registercontainer.h"
#include "ui/theme/theme_manager.h"
#include "utils/to_index.h"
#include <QListWidget>
#include <QMainWindow>
#include <QStackedWidget>
#include <QThread>
#include <QWidget>

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

    //not using enum class here because we want to use the enum values as array indices
    // and if we using enum class we will have to static cast them to size_t/int every time
    enum TabIndex 
    {
        EditorTabIndex = 0,
        MemoryTabIndex,
        ProcessorTabIndex,
        CacheTabIndex,
        CompilerTabIndex,
        ProfilerTabIndex,
        TabCount
    };

    enum class ExecutionState
    {
        Running,
        Stopped,
        Paused
    };

public:
    MainWindow(QWidget *parent = nullptr);
    void setUpUI();
    ~MainWindow();

private:
    void setUpStatusBar();
    void setUpToolBar();
    void setUpSidebar();
    void setUpMenubar();
    void setUpTabs();
    void toggleTheme(const QString &themeId);
    bool tryParseAndLoadProgram();
    void run();
    void processorChangeDialog();
    Ui::MainWindow *ui;

    // QToolBar *m_mainToolbar = nullptr;
    QListWidget       *m_sidebar           {nullptr};
    QStackedWidget    *m_stackedTabs       {nullptr};
    RegisterContainer *m_registerContainer {nullptr};
    ProcessorManager  *m_processorManager  {nullptr};
    QThread           *m_processorThread   {nullptr};
    TabIndex           m_currentTabIndex   {TabIndex::EditorTabIndex};
    ExecutionState     m_executionState    {ExecutionState::Stopped};

    std::array<KitesTab*, toIndex(TabIndex::TabCount)> m_tabs;
public slots:
    void processorChanged(const ProcessorType &vmType); // this will catch the signal from processor dialog
    void runFinishedSlot();
    void themeChangedSlot(const QString &themeId);
    // void runErrorSlot();
signals:
    void processorChangedSignal();
    void runProcessorSignal();
};
} // namespace Kites
