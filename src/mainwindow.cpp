#include "mainwindow.h"

#include "ui_mainwindow.h"
#include "../include/assembler/assembler.h"
#include "../include/assembler/assembler.h"
#include "../include/assembler/code_generator.h"
#include "../include/vm_asm_mw.h"
#include "../include/utils.h"
#include "vm/vm_base.h"
#include "ui/editortab.h"
#include "ui/memorytab.h"
#include "ui/processortab.h"
#include "ui/cachetab.h"
#include "ui/compilertab.h"
#include "ui/profilertab.h"
#include "ui/processor_dialog.h"
#include "ui/settings_dialog.h"
#include "vm/vm_manager.h"
#include "globals.h"
#include <QActionGroup>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QFileDialog>
#include <QWidgetAction>
namespace Kites


{
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
//ui(new Ui::MainWindow)
{
    //ui->setuoUi(this);
    setWindowTitle("Kites RISC-V Simulator");
    setWindowIcon(QIcon(":/icons/kite.png"));
    setUpPalettes();
    toggleTheme(Theme::Light);
    setupVmStateDirectory();


    // well run the vm in a separate thread to keep the ui responsive
    m_vmManager = new VMManager();
    m_vmThread = new QThread(this);
    m_vmManager->moveToThread(m_vmThread);
    m_vmThread->start();

    connect(this,&MainWindow::runVMSignal,m_vmManager,&VMManager::runSlot);
    // well temporarily disable the toolbar buttons when vm is running
    connect(m_vmManager,&VMManager::runFinishedSignal,this,&MainWindow::runFinishedSlot);
    connect(m_vmManager,&VMManager::runErrorSignal,this,[this](const QString& errorMessage){
        //first we enable the toolbar buttons
        runFinishedSlot();
        QMessageBox::critical(this,"Runtime Error",errorMessage + 
        "\nIf you are using floating point instruciction with 5 cycle VM please switch to single cycles VM in processor settings." + 
        "\nWe will include floating point support in multi cycle VM in future releases.");
    });
    connect(m_vmManager,&VMManager::vmStageChangedSignal,this,[this](const QMap<QString,QVariant>& vmState){
        // forward the signal to the processor tab and editor tab to highliht pc line
        /* auto processorTab = dynamic_cast<ProcessorTab*>(m_tabs[TabIndex::ProcessorTabIndex]);
        if(processorTab)
        {
            processorTab->updateVMState(vmState);
        } */
        qDebug() << "MainWindow received vm state change signal";
        auto editorTab = dynamic_cast<EditorTab*>(m_tabs[TabIndex::EditorTabIndex]);
        if(editorTab)
        {
            // -1 is default value meaning no line to highlight
            QVariantMap editorLines = vmState.value("EditorLines",{}).toMap();
            QVariantMap disassemblyLines = vmState.value("DisassemblyLines",{}).toMap();
            

            editorTab->highlightLines(editorLines,disassemblyLines);
        }
    });

    m_registerContainer = new RegisterContainer(this,m_vmManager->getRegisterFile());
    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    QSplitter *splitter = new QSplitter(Qt::Horizontal,this);

    m_stackedTabs = new QStackedWidget(this);
    m_sidebar = new QListWidget(this);
    setUpSidebar();
    setUpToolBar();
    setUpMenubar();
    setUpTabs();
    
    //m_registerContainer = new RegisterContainer(this);
    mainLayout->addWidget(m_sidebar);
    
    splitter->addWidget(m_stackedTabs);
    splitter->addWidget(m_registerContainer);
    splitter->widget(1)->setMaximumWidth(350);

    mainLayout->addWidget(splitter);
    mainLayout->setStretchFactor(m_sidebar, 1);
    mainLayout->setStretchFactor(splitter, 4);
    
    setCentralWidget(central);
    resize(1200, 800);
}

void MainWindow::setUpToolBar()
{
    QToolBar *toolbar = addToolBar("Main Toolbar");
    QAction *processorAction = new QAction("Processor",this);
    QAction *runAction = new QAction("Run", this);
    QAction *pauseAction = new QAction("Pause",this);
    QAction *debugAction = new QAction("Debug Run",this);
    QAction *stepAction = new QAction("Step", this);
    QAction *undoAction = new QAction("Undo", this);
    QAction *redoAction = new QAction("Redo", this);
    
    
    stepAction->setDisabled(true);
    pauseAction->setDisabled(true); 
    undoAction->setDisabled(true);
    redoAction->setDisabled(true);

    //------------Spinbox for setting execution speed----------------
    QSpinBox *spinbox = new QSpinBox(this);
    spinbox->setRange(1,10000);
    spinbox->setValue(1000);
    spinbox->setSuffix(" ms");
    spinbox->setToolTip("Set Execution Speed (in milliseconds)");
    //---------------------------------------------------------------

    QWidgetAction *spinboxAction = new QWidgetAction(this);
    spinboxAction->setDefaultWidget(spinbox);
    toolbar->addAction(processorAction);
    toolbar->addAction(runAction);
    toolbar->addAction(debugAction);
    toolbar->addAction(spinboxAction);
    toolbar->addAction(pauseAction);
    toolbar->addAction(stepAction);
    toolbar->addSeparator();
    toolbar->addAction(undoAction);
    toolbar->addAction(redoAction);
    

    connect(runAction,&QAction::triggered,this,[this,processorAction,
    runAction,pauseAction,debugAction](){
        if(runAction->text() == "Run")
        {
            debugAction->setDisabled(true);
            processorAction->setDisabled(true);
            pauseAction->setEnabled(true);
            runAction->setText("Stop");
            this->run();
        }
        else if(runAction->text() == "Stop")
        {
            //emit stopSignal();
            runAction->setText("Run");
            m_vmManager->stop();
        }
        
    });
    connect(processorAction,&QAction::triggered,this,&MainWindow::processorChangeDialog);
    connect(spinbox,qOverload<int>(&QSpinBox::valueChanged),this,[this](int value){
        m_vmManager->setStepDelay(value);
    });
    connect(pauseAction,&QAction::triggered,this,[this,pauseAction](){
        if(pauseAction->text() == "Pause")
        {
            pauseAction->setText("Resume");
            m_vmManager->pause();
        }
        else
        {
            pauseAction->setText("Pause");
            m_vmManager->resume();
        }
    });

    
    // we also connect the vm paused at breakpoint signal to change the pause button text
    connect(m_vmManager,&VMManager::vmPausedAtBreakpointSignal,this,[this,pauseAction](){
        pauseAction->setText("Resume");
    });

    //connect(stepAction,&QAction::triggered,this,&MainWindow::step);
    connect(undoAction,&QAction::triggered,m_vmManager,&VMManager::undo);
    connect(redoAction,&QAction::triggered,m_vmManager,&VMManager::redo);
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
    //m_sidebar->setFocusPolicy(Qt::ClickFocus);

    connect(m_sidebar, &QListWidget::currentRowChanged, m_stackedTabs, &QStackedWidget::setCurrentIndex);

    QPalette p = m_sidebar->palette();

    p.setColor(QPalette::Highlight, QColor("#2ecc71"));
    p.setColor(QPalette::HighlightedText, Qt::white);

    m_sidebar->setPalette(p);
}

void MainWindow::setUpMenubar()
{
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QMenu *settingsMenu = menuBar()->addMenu("&Settings");
    QMenu *helpMenu = menuBar()->addMenu("&Help");

    QMenu *preferencesMenu = new QMenu("Preferences",this);
    QMenu *processorMenu = new QMenu("Processor",this);
    QAction *openAction = new QAction("Open", this);
    QAction *saveAction = new QAction("Save", this);    
    QAction *exitAction = new QAction("Exit", this);
    //QAction *preferencesAction = new QAction("Preferences", this);
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
    connect(wireStayActiveAction,&QAction::toggled,this,[this](bool checked){
        auto processorTab = dynamic_cast<ProcessorTab*>(m_tabs[TabIndex::ProcessorTabIndex]);
        if(processorTab)
        {
            processorTab->setWiresStayActive(checked);
        }
    });
//--------------------------------------------------
    QAction *advancedSettingsAction = new QAction("Advanced Settings", this);
    connect(advancedSettingsAction,&QAction::triggered,this,[this](){
        SettingsDialog dialog(this);
        dialog.setWindowTitle("Advanced Processor Settings");
        dialog.exec();
    });
    settingsMenu->addAction(advancedSettingsAction);
///////////Help Menu///////////////////
    helpMenu->addAction(aboutAction);
    
    connect(lightThemeAction, &QAction::triggered, this, [this]()
    {
        toggleTheme(Theme::Light);
    });
    connect(darkThemeAction, &QAction::triggered, this, [this]()
    {
        toggleTheme(Theme::Dark);
    });

    connect(openAction, &QAction::triggered, this, [this]()
    {
        QString filename  = QFileDialog::getOpenFileName(this, "Open File", "", "Assembly Files (*.asm *.s);;All Files (*)");

        if(!filename.isEmpty())
        {
            QFile file(filename);
            if(file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QTextStream in(&file);
                QString content = in.readAll();
                auto editorTab = dynamic_cast<EditorTab*>(m_tabs[TabIndex::EditorTabIndex]);
                if(editorTab)
                {
                    editorTab->setRawText(content);
                }
                file.close();
            }
        }
    });

    connect(saveAction, &QAction::triggered, this, [this]()
    {
        QString filename  = QFileDialog::getSaveFileName(this, "Save File", "", "Assembly Files (*.asm *.s);;All Files (*)");

        if(!filename.isEmpty())
        {
            QFile file(filename);
            if(file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream out(&file);
                auto editorTab = dynamic_cast<EditorTab*>(m_tabs[TabIndex::EditorTabIndex]);
                if(editorTab)
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
    m_tabs[TabIndex::EditorTabIndex] = new EditorTab(this);
    m_tabs[TabIndex::MemoryTabIndex] = new MemoryTab(this,m_vmManager->getMemoryController());
    m_tabs[TabIndex::ProcessorTabIndex] = new ProcessorTab(this,m_vmManager); //will add later
    m_tabs[TabIndex::CacheTabIndex] = new CacheTab(this,m_vmManager->getMemoryController());
    m_tabs[TabIndex::CompilerTabIndex] = new CompilerTab(this);
    m_tabs[TabIndex::ProfilerTabIndex] = new ProfilerTab(this);
    //a little experiment 
    connect(this,&MainWindow::vmChangedSignal,
            dynamic_cast<ProcessorTab*>(m_tabs[TabIndex::ProcessorTabIndex]),
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
    auto editor = dynamic_cast<EditorTab*>(m_tabs[TabIndex::EditorTabIndex]);
    editor->resetErrorLines(); // we reset previous error lines
    editor->setCanWrite(false); // we disable writing in editor while vm is running
    m_vmManager->reset();
    // reset register container and memory view as well
    try
    {
        std::string rawText = editor->getRawText();
        std::ofstream out(globals::temporary_assembly_file_path);
        out << rawText;
        out.close();
        AssembledProgram assembledProgram = assemble(globals::temporary_assembly_file_path.string());
        DumpDisasssembly(globals::disassembly_file_path, assembledProgram);
        std::ifstream in(globals::disassembly_file_path);
        std::stringstream buffer;
        buffer << in.rdbuf();
        editor->updateDisassemblyView(buffer.str());
        m_vmManager->loadProgram(assembledProgram);
        m_vmManager->setBreakpoints(editor->getBreakpoints());
        return true;
    }
    catch(const std::exception& e)
    {
        editor->setErrorLinesFromFile(globals::errors_dump_file_path);
        QMessageBox::critical(this, "Error", e.what());
        return false;
    }
}

void MainWindow::run()
{   
    //as soon s as run is clicked we disable the toolbar buttons
    //disableToolBarButtons();
    if (tryParseAndLoadProgram()) 
    {
        // qDebug() << "Starting VM Run";
        // m_vmManager->run();
        emit runVMSignal();
        // qDebug() << "VM Run Completed";
    }
    else
    {
        // if parsing or loading failed we re-enable the buttons
        //otherwise if the vm starts running the buttons will be re-enabled when vm finishes
        runFinishedSlot();
    }
}
void MainWindow::processorChangeDialog()
{
    ProcessorDialog dialog(this,m_vmManager->getVMType());
    dialog.setWindowTitle("Choose Processor");    
    connect(&dialog,&ProcessorDialog::vmSelected,this,&MainWindow::vmChanged);
    dialog.exec();
}

void MainWindow::vmChanged(const VMType& vmType)
{
    //m_vmManager->setVMType(vmType);
    //m_registerContainer->setRegisterFile(m_vmManager->getRegisterFile());
    // this looks kinda ugly but well its better than emitting multiple signals
    qDebug() << "VM Changed to " << static_cast<int>(vmType);
    m_vmManager->changeVM(vmType);
    m_registerContainer->setRegisterFile(m_vmManager->getRegisterFile());
    auto* memtab = dynamic_cast<MemoryTab*>(m_tabs[TabIndex::MemoryTabIndex]);
    memtab->changeMemoryController(m_vmManager->getMemoryController());
    emit vmChangedSignal();

    //well also have to change the processor desing from here later
}


void MainWindow::runFinishedSlot()
{
    auto* editor  = dynamic_cast<EditorTab*>(m_tabs[TabIndex::EditorTabIndex]);
    editor->setCanWrite(true); // re-enable writing in editor when vm stops
    editor->clearHighlights(); // we clear any highlights when vm stops
    //other we are not alble to move the cursor as the paint
    // keeps jumping to last highlighted line
    QList<QToolBar*> toolbars = this->findChildren<QToolBar*>();
    // since we only have one toolbar we can directly access it
    for (QAction* action : toolbars[0]->actions()) 
    {
        if (action->text() == "Run" || action->text() == "Processor"
        || action->text() == "Debug Run")
        {
            action->setEnabled(true);
        }
        if(action->text() == "Stop")
        {
            action->setText("Run");
            // when the program finishes we set the run button text back to run
        }
        if(action->text() == "Resume")
        {
            action->setText("Pause");
            // in case we stop while paused
        }

        if(action->text() == "Pause" 
        || action->text() == "Stop")
        {
            action->setDisabled(true);
        }
    }

    //we also reset the 
}



void MainWindow::setUpPalettes()
{
    // m_palettes[Theme::Light].setColor(QPalette::Window, Qt::white);
    // m_palettes[Theme::Light].setColor(QPalette::WindowText, Qt::black);
    // m_palettes[Theme::Light].setColor(QPalette::Base, QColor(245, 245, 245));
    // m_palettes[Theme::Light].setColor(QPalette::AlternateBase, Qt::white);
    // m_palettes[Theme::Light].setColor(QPalette::ToolTipBase, Qt::white);
    // m_palettes[Theme::Light].setColor(QPalette::ToolTipText, Qt::black);
    // m_palettes[Theme::Light].setColor(QPalette::Text, Qt::black);
    // m_palettes[Theme::Light].setColor(QPalette::Button, Qt::white);
    // m_palettes[Theme::Light].setColor(QPalette::ButtonText, Qt::black);
    // m_palettes[Theme::Light].setColor(QPalette::Highlight, QColor(42, 130, 218));
    // m_palettes[Theme::Light].setColor(QPalette::HighlightedText, Qt::white);
    // // Disabled state for Light theme
    // m_palettes[Theme::Light].setColor(QPalette::Disabled, QPalette::Button, QColor(220, 220, 220)); // light grey
    // m_palettes[Theme::Light].setColor(QPalette::Disabled, QPalette::ButtonText, QColor(150, 150, 150)); // dark grey text
    // m_palettes[Theme::Light].setColor(QPalette::Disabled, QPalette::Text, QColor(150, 150, 150));
    // m_palettes[Theme::Light].setColor(QPalette::Disabled, QPalette::WindowText, QColor(150, 150, 150));


    // // --- Define the Dark Palette ---
    // m_palettes[Theme::Dark].setColor(QPalette::Window, QColor(53, 53, 53));
    // m_palettes[Theme::Dark].setColor(QPalette::WindowText, Qt::white);
    // m_palettes[Theme::Dark].setColor(QPalette::Base, QColor(25, 25, 25));
    // m_palettes[Theme::Dark].setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    // m_palettes[Theme::Dark].setColor(QPalette::Text, Qt::white);
    // m_palettes[Theme::Dark].setColor(QPalette::Button, QColor(53, 53, 53));
    // m_palettes[Theme::Dark].setColor(QPalette::ButtonText, Qt::white);
    // m_palettes[Theme::Dark].setColor(QPalette::Highlight, QColor(42, 130, 218));
    // m_palettes[Theme::Dark].setColor(QPalette::HighlightedText, Qt::black);

    // // Disabled state for Dark theme
    // m_palettes[Theme::Dark].setColor(QPalette::Disabled, QPalette::Button, QColor(70, 70, 70)); // subtle grey
    // m_palettes[Theme::Dark].setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120)); // lighter grey
    // m_palettes[Theme::Dark].setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 120));
    // m_palettes[Theme::Dark].setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 120));

    {
        QPalette& p = m_palettes[Theme::Light];

        // Active / normal groups
        p.setColor(QPalette::Window,          QColor(0xFF, 0xFF, 0xFF));
        p.setColor(QPalette::WindowText,      QColor(0x1A, 0x1A, 0x1A));
        p.setColor(QPalette::Base,            QColor(0xF5, 0xF5, 0xF5));
        p.setColor(QPalette::AlternateBase,   QColor(0xFF, 0xFF, 0xFF));
        p.setColor(QPalette::ToolTipBase,     QColor(0xFF, 0xFF, 0xFF));
        p.setColor(QPalette::ToolTipText,     QColor(0x1A, 0x1A, 0x1A));
        p.setColor(QPalette::Text,            QColor(0x1A, 0x1A, 0x1A));
        p.setColor(QPalette::Button,          QColor(0xF0, 0xF0, 0xF0));
        p.setColor(QPalette::ButtonText,      QColor(0x1A, 0x1A, 0x1A));
        p.setColor(QPalette::Highlight,       QColor(0x2A, 0x82, 0xDA));
        p.setColor(QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF));
        p.setColor(QPalette::Link,            QColor(0x2A, 0x82, 0xDA));

        // Disabled group
        p.setColor(QPalette::Disabled, QPalette::Window,     QColor(0xEC, 0xEC, 0xEC));
        p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x96, 0x96, 0x96));
        p.setColor(QPalette::Disabled, QPalette::Base,       QColor(0xEC, 0xEC, 0xEC));
        p.setColor(QPalette::Disabled, QPalette::Text,       QColor(0x96, 0x96, 0x96));
        p.setColor(QPalette::Disabled, QPalette::Button,     QColor(0xDC, 0xDC, 0xDC));
        p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x96, 0x96, 0x96));

        p.setColor(QPalette::Inactive, QPalette::Highlight, Qt::transparent);
        p.setColor(QPalette::Inactive, QPalette::HighlightedText, Qt::black);
    }

    // ── Dark ──────────────────────────────────────────────────────────────
    {
        QPalette& p = m_palettes[Theme::Dark];

        p.setColor(QPalette::Window,          QColor(0x35, 0x35, 0x35));
        p.setColor(QPalette::WindowText,      QColor(0xF0, 0xF0, 0xF0));
        p.setColor(QPalette::Base,            QColor(0x19, 0x19, 0x19));
        p.setColor(QPalette::AlternateBase,   QColor(0x25, 0x25, 0x25));
        p.setColor(QPalette::ToolTipBase,     QColor(0x45, 0x45, 0x45));
        p.setColor(QPalette::ToolTipText,     QColor(0xF0, 0xF0, 0xF0));
        p.setColor(QPalette::Text,            QColor(0xF0, 0xF0, 0xF0));
        p.setColor(QPalette::Button,          QColor(0x45, 0x45, 0x45));
        p.setColor(QPalette::ButtonText,      QColor(0xF0, 0xF0, 0xF0));
        p.setColor(QPalette::Highlight,       QColor(0x2A, 0x82, 0xDA));
        p.setColor(QPalette::HighlightedText, QColor(0x00, 0x00, 0x00));
        p.setColor(QPalette::Link,            QColor(0x2A, 0x82, 0xDA));

        // Disabled group
        p.setColor(QPalette::Disabled, QPalette::Window,     QColor(0x2A, 0x2A, 0x2A));
        p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x78, 0x78, 0x78));
        p.setColor(QPalette::Disabled, QPalette::Base,       QColor(0x2A, 0x2A, 0x2A));
        p.setColor(QPalette::Disabled, QPalette::Text,       QColor(0x78, 0x78, 0x78));
        p.setColor(QPalette::Disabled, QPalette::Button,     QColor(0x46, 0x46, 0x46));
        p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x78, 0x78, 0x78));
    }

}

QString loadStyleSheet(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ThemeManager: failed to load stylesheet:" << path;
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

void MainWindow::toggleTheme(Theme theme)
{
    QApplication::setPalette(m_palettes[theme]);

    // const QString path = (theme == Theme::Dark)
    //                      ? QStringLiteral(":/themes/dark.qss")
    //                      : QStringLiteral(":/themes/light.qss");

    // qApp->setStyleSheet(loadStyleSheet(path));
}

MainWindow::~MainWindow()
{
    m_vmManager->stop();
    //delete ui;
}
}// namespace Kites
