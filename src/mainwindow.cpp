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
namespace Kites
{
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
//ui(new Ui::MainWindow)
{
    //ui->setuoUi(this);
    setWindowTitle("Kites RISC-V Simulator");
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
    splitter->widget(1)->setMaximumWidth(300);
    mainLayout->addWidget(splitter);
    mainLayout->setStretchFactor(m_sidebar, 1);
    mainLayout->setStretchFactor(splitter, 4);
    setCentralWidget(central);
    resize(1200, 800);
    //setUpUI();
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
    QAction *openAction = new QAction("Open", this);
    QAction *saveAction = new QAction("Save", this);
    QAction *exitAction = new QAction("Exit", this);
    QAction *preferencesAction = new QAction("Preferences", this);
    QAction *aboutAction = new QAction("About", this);

    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    settingsMenu->addAction(preferencesAction);
    helpMenu->addAction(aboutAction);

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
    m_tabs[TabIndex::MemoryTabIndex] = new MemoryTab(this);
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
// void MainWindow::setUpUI()
// {
   


//     //sidebar
    

//     //Editor View
//     QStackedWidget *stack = new QStackedWidget();
//     QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);

//     // === Left side: text editor ===
//     QTextEdit *editor = new QTextEdit();
//     editor->setPlaceholderText("Write your assembly code here...");
//     mainSplitter->addWidget(editor);

//     // === Right side: vertical splitter for registers + disassembly ===
//     //QSplitter *rightSplitter = new QSplitter(Qt::Vertical, this);

//     // Register view
//     QTableWidget *registerTable = new QTableWidget(32, 2);
//     registerTable->setHorizontalHeaderLabels({"Register", "Value"});
//     for (int i = 0; i < 32; ++i) {
//         registerTable->setItem(i, 0, new QTableWidgetItem(QString("x%1").arg(i)));
//         registerTable->setItem(i, 1, new QTableWidgetItem("0x00000000"));
//     }

//     // Disassembly view
//     QTextEdit *disassemblyView = new QTextEdit();
//     disassemblyView->setReadOnly(true);
//     disassemblyView->setPlaceholderText("Disassembled code will appear here...");

//     // Add both to right splitter
//     mainSplitter->addWidget(disassemblyView);
//     mainSplitter->addWidget(registerTable);

//     // Add the right splitter to main splitter
//     //mainSplitter->addWidget(rightSplitter);

//     // Optional: set initial sizes (ratio)
//     mainSplitter->setStretchFactor(0, 3);
//     mainSplitter->setStretchFactor(1, 2);

//     stack->addWidget(mainSplitter);

//     //Memory View
//     QTextEdit *memoryView = new QTextEdit();
//     memoryView->setPlaceholderText("This will show the memory");
//     stack->addWidget(memoryView);

//     //adding to Layout

//     mainLayout->addWidget(sidebar);
//     mainLayout->addWidget(stack);


//     setCentralWidget(central);
//     setWindowTitle("RISC-V Visual Assembler");
//

//     // === Create the menu bar ===
    

//     // === Create a top toolbar ===
  

//     // === Connect actions ===

    

// }


MainWindow::~MainWindow()
{
    delete ui;
}
}// namespace Kites
