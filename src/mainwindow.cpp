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
#include "ui/processor_dialog.h"
#include "vm/vm_manager.h"
#include "globals.h"
#include <QActionGroup>
#include <QHBoxLayout>
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
    m_vmManager = std::make_unique<VMManager>(this);
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
    // splitter->setStretchFactor(0, 2);
    // splitter->setStretchFactor(1, 1);
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
    QAction *stepAction = new QAction("Step", this);
    //toolbar->addAction(preferencesAction);
    toolbar->addAction(processorAction);
    toolbar->addAction(runAction);
    toolbar->addAction(stepAction);
    
    connect(runAction,&QAction::triggered,this,&MainWindow::run);
    connect(processorAction,&QAction::triggered,this,&MainWindow::processorChangeDialog);
    //connect(stepAction,&QAction::triggered,this,&MainWindow::step);
}

void MainWindow::setUpSidebar()
{
    m_sidebar->addItem("Editor");
    m_sidebar->addItem("Memory");
    m_sidebar->addItem("Processor");
    m_sidebar->setFixedWidth(80);
    m_sidebar->setCurrentRow(0);

    connect(m_sidebar, &QListWidget::currentRowChanged, m_stackedTabs, &QStackedWidget::setCurrentIndex);
}

void MainWindow::setUpMenubar()
{
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QMenu *settingsMenu = menuBar()->addMenu("&Settings");
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QMenu *preferencesMenu = new QMenu("Preferences",this);

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

///////////Help Menu///////////////////
    helpMenu->addAction(aboutAction);
    

    connect(lightThemeAction, &QAction::triggered, this, [this]() {
        toggleTheme(Theme::Light);
    });
    connect(darkThemeAction, &QAction::triggered, this, [this]() {
        toggleTheme(Theme::Dark);
    });

}

void MainWindow::setUpTabs()
{
    m_tabs[TabIndex::EditorTabIndex] = new EditorTab(this);
    m_tabs[TabIndex::MemoryTabIndex] = new MemoryTab(this,m_vmManager->getMemoryController());
    m_tabs[TabIndex::ProcessorTabIndex] = new ProcessorTab(this,m_vmManager.get()); //will add later

    //a little experiment 
    connect(this,&MainWindow::vmChangedSignal,
            dynamic_cast<ProcessorTab*>(m_tabs[TabIndex::ProcessorTabIndex]),
            &ProcessorTab::onVMChanged);

    m_stackedTabs->addWidget(m_tabs[TabIndex::EditorTabIndex]);
    m_stackedTabs->addWidget(m_tabs[TabIndex::MemoryTabIndex]);
    m_stackedTabs->addWidget(m_tabs[TabIndex::ProcessorTabIndex]);
}
bool MainWindow::tryParseAndLoadProgram()
{
    auto editor = dynamic_cast<EditorTab*>(m_tabs[TabIndex::EditorTabIndex]);
    editor->resetErrorLines(); // we reset previous error lines
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
    if (tryParseAndLoadProgram()) 
    {
        m_vmManager->run();
    }
}
void MainWindow::processorChangeDialog()
{
    ProcessorDialog dialog(this,m_vmManager->getVMType());
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

void MainWindow::setUpPalettes()
{
    m_palettes[Theme::Light].setColor(QPalette::Window, Qt::white);
    m_palettes[Theme::Light].setColor(QPalette::WindowText, Qt::black);
    m_palettes[Theme::Light].setColor(QPalette::Base, QColor(245, 245, 245));
    m_palettes[Theme::Light].setColor(QPalette::AlternateBase, Qt::white);
    m_palettes[Theme::Light].setColor(QPalette::ToolTipBase, Qt::white);
    m_palettes[Theme::Light].setColor(QPalette::ToolTipText, Qt::black);
    m_palettes[Theme::Light].setColor(QPalette::Text, Qt::black);
    m_palettes[Theme::Light].setColor(QPalette::Button, Qt::white);
    m_palettes[Theme::Light].setColor(QPalette::ButtonText, Qt::black);
    m_palettes[Theme::Light].setColor(QPalette::Highlight, QColor(42, 130, 218));
    m_palettes[Theme::Light].setColor(QPalette::HighlightedText, Qt::white);

    // --- Define the Dark Palette ---
    m_palettes[Theme::Dark].setColor(QPalette::Window, QColor(53, 53, 53));
    m_palettes[Theme::Dark].setColor(QPalette::WindowText, Qt::white);
    m_palettes[Theme::Dark].setColor(QPalette::Base, QColor(25, 25, 25));
    m_palettes[Theme::Dark].setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    m_palettes[Theme::Dark].setColor(QPalette::Text, Qt::white);
    m_palettes[Theme::Dark].setColor(QPalette::Button, QColor(53, 53, 53));
    m_palettes[Theme::Dark].setColor(QPalette::ButtonText, Qt::white);
    m_palettes[Theme::Dark].setColor(QPalette::Highlight, QColor(42, 130, 218));
    m_palettes[Theme::Dark].setColor(QPalette::HighlightedText, Qt::black);
}

void MainWindow::toggleTheme(Theme theme)
{
    QApplication::setPalette(m_palettes[theme]);
}

MainWindow::~MainWindow()
{
    delete ui;
}
}// namespace Kites
