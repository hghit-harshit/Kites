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
#include <QActionGroup>
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
    m_vmManager = new VMManager(this);
    //VMManager::getInstance(); //will initialize the VMManager singleton
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
    //toolbar->addAction(preferencesAction);
    toolbar->addAction(runAction);
    toolbar->addAction(processorAction);


    connect(runAction,&QAction::triggered,this,&MainWindow::run);
    connect(processorAction,&QAction::triggered,this,&MainWindow::processorChangeDialog);
    // connect(runAction,&QAction::triggered, this, [=](){
    //     std::string code = editor->toPlainText().toStdString();
    //     //std::string asmcode = code.toStdString();
    //     std::string tempFile = "temp.asm";

    //     std::ofstream out(tempFile);
    //     out << code;
    //     out.close();

    //     AssembledProgram asmprog = assemble(tempFile);
    //     std::vector<uint32_t> disassembledCode = generateMachineCode(asmprog.intermediate_code);
    //     QString disassemblyText;
    //     for (const auto& line : disassembledCode) {
    //         std::ostringstream oss;
    //         oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << line;
    //         disassemblyText += oss.str() + "\n";
    //     }
    //     disassemblyView->setPlainText(disassemblyText);

    // });
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
   
    // connect(openAction, &QAction::triggered, this, [=]() {
    //     QString fileName = QFileDialog::getOpenFileName(this, "Open Assembly File", "", "Assembly Files (*.s *.asm);;All Files (*)");
    //     if (!fileName.isEmpty()) {
    //         QFile file(fileName);
    //         if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    //             editor->setPlainText(file.readAll());
    //     }
    // });

    // connect(saveAction, &QAction::triggered, this, [=]() {
    //     QString fileName = QFileDialog::getSaveFileName(this, "Save Assembly File", "", "Assembly Files (*.s *.asm);;All Files (*)");
    //     if (!fileName.isEmpty()) {
    //         QFile file(fileName);
    //         if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    //             file.write(editor->toPlainText().toUtf8());
    //     }
    // });

    // connect(exitAction, &QAction::triggered, this, &MainWindow::close);

    // connect(aboutAction, &QAction::triggered, this, [=]() {
    //     QMessageBox::about(this, "About", "RISC-V Visual Assembler\nBuilt with Qt");
    // });
}

void MainWindow::setUpTabs()
{
    m_tabs[TabIndex::EditorTabIndex] = new EditorTab(this);
    m_tabs[TabIndex::MemoryTabIndex] = new MemoryTab(this,m_vmManager->getMemoryController());
    m_tabs[TabIndex::ProcessorTabIndex] = new ProcessorTab(this); //will add later

    m_stackedTabs->addWidget(m_tabs[TabIndex::EditorTabIndex]);
    m_stackedTabs->addWidget(m_tabs[TabIndex::MemoryTabIndex]);
    m_stackedTabs->addWidget(m_tabs[TabIndex::ProcessorTabIndex]);
}
void MainWindow::run()
{
    //will change this later for now we just want to compile
    m_vmManager->reset();
    auto editor = dynamic_cast<EditorTab*>(m_tabs[TabIndex::EditorTabIndex]);
    std::string rawText = editor->getRawText();
    std::string tempFile = "temp.asm";
    std::ofstream out(tempFile);
    out << rawText;
    out.close();
    std::string disassemblyTextFile = "disassembly.txt";
    AssembledProgram assembledProgram = assemble(tempFile);
    DumpDisasssembly(disassemblyTextFile,assembledProgram);
    std::ifstream in(disassemblyTextFile);
    std::stringstream buffer;
    buffer << in.rdbuf();
    editor->updateDisassemblyView(buffer.str());

    m_vmManager->loadProgram(assembledProgram);
    m_vmManager->run();
}

void MainWindow::processorChangeDialog()
{
    ProcessorDialog dialog(this);
    dialog.exec();
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
