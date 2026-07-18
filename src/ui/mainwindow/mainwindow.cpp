#include "mainwindow.h"
#include "assembler/assembler.h"
#include "common/assembled_program.h"
#include "common/globals.h"
#include "custom_pseudo_manager/custom_pseudo_manager.h"
#include "processor/processor_manager.h"
#include "ui/cache_tab/cachetab.h"
#include "ui/compiler_tab/compilertab.h"
#include "ui/dialogs/processor_dialog.h"
#include "ui/dialogs/settings_dialog.h"
#include "ui/editor_tab/editortab.h"
#include "ui/memory_tab/memorytab.h"
#include "ui/processor_tab/processortab.h"
#include "ui/profiler_tab/profilertab.h"
#include "ui/theme/icon_manager.h"
#include "ui_mainwindow.h"
#include "utils/utils.h"
#include <QActionGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QWidgetAction>
#include <QSplitter>
#include <QMessageBox>

namespace Kites
{
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
// ui(new Ui::MainWindow)
{
    // ui->setuoUi(this);
    setWindowTitle("Kites RISC-V Simulator");
    setWindowIcon(QIcon(":/icons/kite.png"));
    toggleTheme(ThemeType::Dark);
    setupVmStateDirectory();

    // well run the vm in a separate thread to keep the ui responsive
    // do not touch these 4 line unless you really know what you are doing
    m_processorManager = new ProcessorManager();
    m_processorThread = new QThread(this);
    m_processorManager->moveToThread(m_processorThread);
    m_processorThread->start();

            
    m_registerContainer = new RegisterContainer(this, m_processorManager->getRegisterFile());
    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    m_stackedTabs = new QStackedWidget(this);
    m_sidebar = new QListWidget(this);
    setUpStatusBar();
    setUpSidebar();
    setUpToolBar();
    setUpMenubar();
    setUpTabs();

    // m_registerContainer = new RegisterContainer(this);
    mainLayout->addWidget(m_sidebar);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    splitter->addWidget(m_stackedTabs);
    splitter->addWidget(m_registerContainer);
    splitter->widget(1)->setMaximumWidth(350);

    mainLayout->addWidget(splitter);
    mainLayout->setStretchFactor(m_sidebar, 1);
    mainLayout->setStretchFactor(splitter, 4);

    setCentralWidget(central);
    resize(1200, 800);
    connect(this, &MainWindow::runProcessorSignal, m_processorManager, &ProcessorManager::runSlot);
    // well temporarily disable the toolbar buttons when vm is running
    connect(m_processorManager, &ProcessorManager::runFinishedSignal, this, &MainWindow::runFinishedSlot);
    connect(m_processorManager, &ProcessorManager::runErrorSignal, this,
            [this](const QString &errorMessage)
            {
                // first we enable the toolbar buttons
                runFinishedSlot();
                QMessageBox::critical(this, "Runtime Error", errorMessage);
            });

    //TODO : instead of using lambda just connect the signal to the fucntions directly
    connect(m_processorManager, &ProcessorManager::updateDisassemblySignal, this,
            [this](const QString &disassemblyText)
            {
                auto editorTab = dynamic_cast<EditorTab *>(m_tabs[TabIndex::EditorTabIndex]);
                if (editorTab)
                {
                    editorTab->updateDisassemblyView(disassemblyText.toStdString());
                }
            });
    connect(m_processorManager, &ProcessorManager::updateEditorHighlightSignal, this,
            [this](const std::vector<std::pair<int,std::string>> &editorLines, 
            const std::vector<std::pair<int,std::string>> &disassemblyLines)
            {
                auto editorTab = dynamic_cast<EditorTab *>(m_tabs[TabIndex::EditorTabIndex]);
                if (editorTab)
                {
                    editorTab->highlightLines(editorLines, disassemblyLines);
                }
            });
    connect(&ThemeManager::getInstance(), &ThemeManager::themeChangedSignal, 
            this, &MainWindow::themeChangedSlot);
}

void MainWindow::setUpStatusBar()
{
    QStatusBar *statusBar = this->statusBar();
    QLabel *statusLabel = new QLabel("Ready", this);
    statusBar->addPermanentWidget(statusLabel);
}

void MainWindow::setUpToolBar()
{
    QToolBar *toolbar         = addToolBar("Main Toolbar");
    QAction  *processorAction = new QAction("Processor", this);
    QAction  *runAction       = new QAction("Run", this);
    QAction  *pauseAction     = new QAction("Pause", this);
    QAction  *debugAction     = new QAction("Debug Run", this);
    QAction  *stepAction      = new QAction("Step", this);
    QAction  *undoAction      = new QAction("Undo", this);
    QAction  *redoAction      = new QAction("Redo", this);

    stepAction->setDisabled(true);
    pauseAction->setDisabled(true);
    undoAction->setDisabled(true);
    redoAction->setDisabled(true);

    // set toolbar icons (resources in :/icons)
    runAction->setIcon(IconManager::getInstance().getIcon(Icon::Play));
    pauseAction->setIcon(IconManager::getInstance().getIcon(Icon::Pause));
    undoAction->setIcon(IconManager::getInstance().getIcon(Icon::Undo));
    redoAction->setIcon(IconManager::getInstance().getIcon(Icon::Redo));
    stepAction->setIcon(IconManager::getInstance().getIcon(Icon::Step));
    // tooltips for accessibility
    processorAction->setToolTip("Processor settings");
    runAction->setToolTip("Run program");
    pauseAction->setToolTip("Pause execution");
    debugAction->setToolTip("Start debug run");
    stepAction->setToolTip("Execute single step");
    undoAction->setToolTip("Undo last VM step");
    redoAction->setToolTip("Redo last VM step");
    // optional: make icons a consistent size
    toolbar->setIconSize(QSize(18, 18));

    //------------Spinbox for setting execution speed----------------
    QSpinBox *spinbox = new QSpinBox(this);
    spinbox->setRange(1, 10000);
    spinbox->setValue(1000);
    spinbox->setSuffix(" ms");
    spinbox->setToolTip("Set Execution Speed (in milliseconds)");
    //---------------------------------------------------------------

    QWidgetAction *spinboxAction = new QWidgetAction(this);
    spinboxAction->setDefaultWidget(spinbox);
    toolbar->addAction(processorAction);
    toolbar->addAction(runAction);
    // toolbar->addAction(debugAction);
    toolbar->addAction(spinboxAction);
    toolbar->addAction(pauseAction);
    toolbar->addAction(stepAction);
    toolbar->addSeparator();
    toolbar->addAction(undoAction);
    toolbar->addAction(redoAction);

    connect(runAction, &QAction::triggered, this,
            [this, processorAction, runAction, pauseAction, debugAction]()
            {
                if (runAction->text() == "Run")
                {
                    debugAction->setDisabled(true);
                    processorAction->setDisabled(true);
                    pauseAction->setEnabled(true);
                    runAction->setText("Stop");
                    runAction->setIcon(IconManager::getInstance().getIcon(Icon::Stop));
                    runAction->setToolTip("Stop execution");
                    this->run();
                }
                else if (runAction->text() == "Stop")
                {
                    // emit stopSignal();
                    runAction->setText("Run");
                    runAction->setIcon(IconManager::getInstance().getIcon(Icon::Play));
                    runAction->setToolTip("Run program");
                    m_processorManager->stop();
                }
            });

    connect(debugAction, &QAction::triggered, this,
            [this, processorAction, runAction, pauseAction, stepAction, undoAction, redoAction,
             debugAction]()
            {
                debugAction->setDisabled(true);
                processorAction->setDisabled(true);
                pauseAction->setEnabled(true);
                stepAction->setEnabled(true);
                undoAction->setEnabled(true);
                redoAction->setEnabled(true);
                // setEditorNavigationEnabled(false);
                auto editorTab = dynamic_cast<EditorTab *>(m_tabs[TabIndex::EditorTabIndex]);
                if (editorTab)
                {
                    editorTab->setExpandedLocked(true);
                }
                m_processorManager->debugRun();
            });

    connect(processorAction, &QAction::triggered, this, &MainWindow::processorChangeDialog);

    connect(spinbox, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) { m_processorManager->setStepDelay(value); });

    connect(pauseAction, &QAction::triggered, this,
            [this, pauseAction, undoAction, redoAction]()
            {
                if (pauseAction->text() == "Pause")
                {
                    pauseAction->setText("Resume");
                    pauseAction->setIcon(IconManager::getInstance().getIcon(Icon::Resume));
                    pauseAction->setToolTip("Resume execution");
                    undoAction->setEnabled(true);
                    redoAction->setEnabled(true);
                    m_processorManager->pause();
                }
                else
                {
                    pauseAction->setText("Pause");
                    pauseAction->setIcon(IconManager::getInstance().getIcon(Icon::Pause));
                    pauseAction->setToolTip("Pause execution");
                    undoAction->setDisabled(true);
                    redoAction->setDisabled(true);
                    m_processorManager->resume();
                }
            });

    // we also connect the vm paused at breakpoint signal to change the pause button text
    connect(m_processorManager, &ProcessorManager::processorPausedAtBreakpointSignal, this,
            [this, pauseAction]()
            {
                pauseAction->setText("Resume");
                pauseAction->setIcon(IconManager::getInstance().getIcon(Icon::Resume));
                pauseAction->setToolTip("Resume execution");
            });

    connect(stepAction, &QAction::triggered, this, [this]() { m_processorManager->step(); });
    connect(undoAction, &QAction::triggered, this, [this]() { m_processorManager->undo(); });
    connect(redoAction, &QAction::triggered, this, [this]() { m_processorManager->redo(); });
}

void MainWindow::setUpSidebar()
{
    m_sidebar->addItem("Editor");
    m_sidebar->addItem("Memory");
    m_sidebar->addItem("Processor");
    m_sidebar->addItem("Cache");
    m_sidebar->addItem("Compiler");
    m_sidebar->addItem("Profiler");
    m_sidebar->setFixedWidth(80);
    m_sidebar->setCurrentRow(0);
    // m_sidebar->setFocusPolicy(Qt::ClickFocus);

    connect(m_sidebar, &QListWidget::currentRowChanged, m_stackedTabs,
            &QStackedWidget::setCurrentIndex);

    QPalette p = m_sidebar->palette();

    // TODO : get this from theme manager
    p.setColor(QPalette::Highlight, QColor("#2ecc71"));
    p.setColor(QPalette::HighlightedText, Qt::white);

    m_sidebar->setPalette(p);
}

void MainWindow::setUpMenubar()
{
    QMenu *fileMenu     = menuBar()->addMenu("&File");
    QMenu *settingsMenu = menuBar()->addMenu("&Settings");
    QMenu *helpMenu     = menuBar()->addMenu("&Help");

    QMenu *preferencesMenu = new QMenu("Preferences", this);
    QMenu *processorMenu   = new QMenu("Processor", this);
    
    QAction *openAction  = new QAction("Open", this);
    QAction *saveAction  = new QAction("Save", this);
    QAction *exitAction  = new QAction("Exit", this);
    QAction *aboutAction = new QAction("About", this);

    ///////////File Menu///////////////////
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);
    ////////////Setting Menu////////////////
    settingsMenu->addMenu(preferencesMenu);
    QAction *lightThemeAction = new QAction("Light", this);
    lightThemeAction->setCheckable(true); // Make it checkable
    lightThemeAction->setChecked(true);
    QAction *darkThemeAction = new QAction("Dark", this);
    darkThemeAction->setCheckable(true); // Make it checkable

    QActionGroup *themeGroup = new QActionGroup(this);
    themeGroup->addAction(lightThemeAction);
    themeGroup->addAction(darkThemeAction);

    preferencesMenu->addAction(lightThemeAction);
    preferencesMenu->addAction(darkThemeAction);
    //----------------------------------------------
    settingsMenu->addMenu(processorMenu);
    QAction *wireStayActiveAction = new QAction("Wires Stay Active", this);
    wireStayActiveAction->setCheckable(true);
    wireStayActiveAction->setChecked(false);
    processorMenu->addAction(wireStayActiveAction);
    connect(wireStayActiveAction, &QAction::toggled, this,
            [this](bool checked)
            {
                auto processorTab =
                    dynamic_cast<ProcessorTab *>(m_tabs[TabIndex::ProcessorTabIndex]);
                if (processorTab)
                {
                    processorTab->setWiresStayActive(checked);
                }
            });
    //--------------------------------------------------
    QAction *advancedSettingsAction = new QAction("Advanced Settings", this);
    connect(advancedSettingsAction, &QAction::triggered, this,
            [this]()
            {
                SettingsDialog dialog(this);
                dialog.setWindowTitle("Advanced Processor Settings");
                dialog.exec();
            });
    settingsMenu->addAction(advancedSettingsAction);
    ///////////Help Menu///////////////////
    helpMenu->addAction(aboutAction);

    connect(lightThemeAction, &QAction::triggered, this, [this]() { toggleTheme(ThemeType::Light); });
    connect(darkThemeAction, &QAction::triggered, this, [this]() { toggleTheme(ThemeType::Dark); });

    connect(openAction, &QAction::triggered, this,
            [this]()
            {
                QString filename = QFileDialog::getOpenFileName(
                    this, "Open File", "", "Assembly Files (*.asm *.s);;All Files (*)");

                if (!filename.isEmpty())
                {
                    QFile file(filename);
                    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                    {
                        QTextStream in(&file);
                        QString content = in.readAll();
                        auto editorTab =
                            dynamic_cast<EditorTab *>(m_tabs[TabIndex::EditorTabIndex]);
                        if (editorTab)
                        {
                            editorTab->setRawText(content);
                        }
                        file.close();
                    }
                }
            });

    connect(saveAction, &QAction::triggered, this,
            [this]()
            {
                QString filename = QFileDialog::getSaveFileName(
                    this, "Save File", "", "Assembly Files (*.asm *.s);;All Files (*)");

                if (!filename.isEmpty())
                {
                    QFile file(filename);
                    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
                    {
                        QTextStream out(&file);
                        auto editorTab =
                            dynamic_cast<EditorTab *>(m_tabs[TabIndex::EditorTabIndex]);
                        if (editorTab)
                        {
                            out << editorTab->getRawText().c_str();
                        }
                        file.close();
                    }
                }
            });
}

void MainWindow::setUpTabs()
{
    m_tabs[TabIndex::EditorTabIndex]    = new EditorTab(this);
    m_tabs[TabIndex::MemoryTabIndex]    = new MemoryTab(this, m_processorManager->getMemoryController());
    m_tabs[TabIndex::ProcessorTabIndex] = new ProcessorTab(this, m_processorManager);                      
    m_tabs[TabIndex::CacheTabIndex]     = new CacheTab(this, m_processorManager->getMemoryController());
    m_tabs[TabIndex::CompilerTabIndex]  = new CompilerTab(this);
    m_tabs[TabIndex::ProfilerTabIndex]  = new ProfilerTab(this, m_processorManager->getProfiler());
    // a little experiment
    connect(this, &MainWindow::processorChangedSignal,
            dynamic_cast<ProcessorTab *>(m_tabs[TabIndex::ProcessorTabIndex]),
            &ProcessorTab::onVMChanged);

    m_stackedTabs->addWidget(m_tabs[TabIndex::EditorTabIndex]);
    m_stackedTabs->addWidget(m_tabs[TabIndex::MemoryTabIndex]);
    m_stackedTabs->addWidget(m_tabs[TabIndex::ProcessorTabIndex]);
    m_stackedTabs->addWidget(m_tabs[TabIndex::CacheTabIndex]);
    m_stackedTabs->addWidget(m_tabs[TabIndex::CompilerTabIndex]);
    m_stackedTabs->addWidget(m_tabs[TabIndex::ProfilerTabIndex]);
}

bool MainWindow::tryParseAndLoadProgram()
{
    auto* editor = dynamic_cast<EditorTab*>(m_tabs[TabIndex::EditorTabIndex]);
    auto* profilerTab = dynamic_cast<ProfilerTab*>(m_tabs[TabIndex::ProfilerTabIndex]);

    if(!editor || !profilerTab) 
    {
        QMessageBox::critical(this, "Error", "Editor or Profiler tab not found.");
        return false;
    }
    editor->switchToExpandedView(); // switch to the expanded view 
                                    //to show expanded pseudoinstructions
    editor->resetErrorLines();    // we reset previous error lines
    editor->setCanWrite(false);   // we disable writing in editor while vm is running
    m_processorManager->reset();
    // reset register container and memory view as well
    try
    {
        std::string rawText = editor->getRawText();
        rawText = customPseudoManager::expandPseudoInstruction(rawText);
        std::ofstream out(globals::temporary_assembly_file_path);
        out << rawText;
        out.close();
        AssembledProgram assembledProgram = assemble(globals::temporary_assembly_file_path.string());
        DumpDisasssembly(globals::disassembly_file_path, assembledProgram);
        std::ifstream in(globals::disassembly_file_path);
        std::stringstream buffer;
        buffer << in.rdbuf();
        editor->updateDisassemblyView(buffer.str());
        profilerTab->setSourceText(QString::fromStdString(rawText));
        
        m_processorManager->loadProgram(assembledProgram);
        m_processorManager->setBreakpoints(editor->getBreakpoints());
        return true;
    }
    catch (const std::exception &e)
    {
        editor->setErrorLinesFromFile(globals::errors_dump_file_path);
        QMessageBox::critical(this, "Error", e.what());
        return false;
    }
}

void MainWindow::run()
{
    // as soon s as run is clicked we disable the toolbar buttons
    // disableToolBarButtons();
    if (tryParseAndLoadProgram())
    {

        auto editorTab = dynamic_cast<EditorTab *>(m_tabs[TabIndex::EditorTabIndex]);
        if (editorTab)
        {
            editorTab->setExpandedLocked(true);
        }

        emit runProcessorSignal();
    }
    else
    {
        // if parsing or loading failed we re-enable the buttons
        // otherwise if the vm starts running the buttons will be re-enabled when vm finishes
        runFinishedSlot();
    }
}
void MainWindow::processorChangeDialog()
{
    ProcessorDialog dialog(this, m_processorManager->getProcessorType());
    dialog.setWindowTitle("Choose Processor");
    connect(&dialog, &ProcessorDialog::vmSelected, this, &MainWindow::processorChanged);
    dialog.exec();
}

void MainWindow::processorChanged(const ProcessorType &vmType)
{
    // m_vmManager->setVMType(vmType);
    // m_registerContainer->setRegisterFile(m_vmManager->getRegisterFile());
    //  this looks kinda ugly but well its better than emitting multiple signals
    qDebug() << "VM Changed to " << static_cast<int>(vmType);
    m_processorManager->changeProcessor(vmType);
    m_registerContainer->setRegisterFile(m_processorManager->getRegisterFile());
    auto *memtab = dynamic_cast<MemoryTab *>(m_tabs[TabIndex::MemoryTabIndex]);
    if (memtab)
    {
        memtab->changeMemoryController(m_processorManager->getMemoryController());
    }
    auto *cacheTab = dynamic_cast<CacheTab *>(m_tabs[TabIndex::CacheTabIndex]);
    if (cacheTab)
    {
        cacheTab->changeMemoryController(m_processorManager->getMemoryController());
    }
    emit processorChangedSignal();

    // well also have to change the processor design from here later
}

void MainWindow::runFinishedSlot()
{
    auto *editor = dynamic_cast<EditorTab *>(m_tabs[TabIndex::EditorTabIndex]);
    editor->setCanWrite(true); // re-enable writing in editor when vm stops
    editor->setExpandedLocked(false);
    editor->clearHighlights(); // we clear any highlights when vm stops
    // other we are not alble to move the cursor as the paint
    //  keeps jumping to last highlighted line
    QList<QToolBar *> toolbars = this->findChildren<QToolBar *>();
    // since we only have one toolbar we can directly access it
    for (QAction *action : toolbars[0]->actions())
    {
        if (action->text() == "Run" || action->text() == "Processor" ||
            action->text() == "Debug Run")
        {
            action->setEnabled(true);
        }
        if (action->text() == "Stop")
        {
            action->setText("Run");
            action->setIcon(IconManager::getInstance().getIcon(Icon::Play));
            action->setToolTip("Run program");
            // when the program finishes we set the run button text back to run
        }
        if (action->text() == "Resume")
        {
            action->setText("Pause");
            action->setIcon(IconManager::getInstance().getIcon(Icon::Pause));
            action->setToolTip("Pause execution");
            // in case we stop while paused
        }

        if (action->text() == "Pause" || action->text() == "Stop")
        {
            action->setDisabled(true);
        }
    }

}


void MainWindow::toggleTheme(ThemeType theme)
{
    ThemeManager::getInstance().setTheme(theme);
}

void MainWindow::themeChangedSlot([[maybe_unused]]ThemeType theme)
{
    // we will update the icons here based on the theme
    // this is very very hacky but this will do for now
    auto *toolbar = this->findChild<QToolBar *>();
    if (toolbar)
    {
        for (QAction *action : toolbar->actions())
        {
            if (action->text() == "Run")
            {
                action->setIcon(IconManager::getInstance().getIcon(Icon::Play));
            }
            else if (action->text() == "Pause")
            {
                action->setIcon(IconManager::getInstance().getIcon(Icon::Pause));
            }
            else if (action->text() == "Resume")
            {
                action->setIcon(IconManager::getInstance().getIcon(Icon::Resume));
            }
            else if (action->text() == "Stop")
            {
                action->setIcon(IconManager::getInstance().getIcon(Icon::Stop));
            }
            else if (action->text() == "Undo")
            {
                action->setIcon(IconManager::getInstance().getIcon(Icon::Undo));
            }
            else if (action->text() == "Redo")
            {
                action->setIcon(IconManager::getInstance().getIcon(Icon::Redo));
            }
            else if (action->text() == "Step")
            {
                action->setIcon(IconManager::getInstance().getIcon(Icon::Step));
            }
        }
    }
}

MainWindow::~MainWindow()
{
    m_processorManager->stop();
    // delete ui;
}
} // namespace Kites
