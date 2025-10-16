#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../include/assembler/assembler.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setUpUI();
}

void MainWindow::setUpUI()
{
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);

    // === Left side: text editor ===
    QTextEdit *editor = new QTextEdit();
    editor->setPlaceholderText("Write your assembly code here...");
    mainSplitter->addWidget(editor);

    // === Right side: vertical splitter for registers + disassembly ===
    //QSplitter *rightSplitter = new QSplitter(Qt::Vertical, this);

    // Register view
    QTableWidget *registerTable = new QTableWidget(32, 2);
    registerTable->setHorizontalHeaderLabels({"Register", "Value"});
    for (int i = 0; i < 32; ++i) {
        registerTable->setItem(i, 0, new QTableWidgetItem(QString("x%1").arg(i)));
        registerTable->setItem(i, 1, new QTableWidgetItem("0x00000000"));
    }

    // Disassembly view
    QTextEdit *disassemblyView = new QTextEdit();
    disassemblyView->setReadOnly(true);
    disassemblyView->setPlaceholderText("Disassembled code will appear here...");

    // Add both to right splitter
    mainSplitter->addWidget(disassemblyView);
    mainSplitter->addWidget(registerTable);

    // Add the right splitter to main splitter
    //mainSplitter->addWidget(rightSplitter);

    // Optional: set initial sizes (ratio)
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 2);

    setCentralWidget(mainSplitter);
    setWindowTitle("RISC-V Visual Assembler");
    resize(1200, 800);

    // === Create the menu bar ===
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QMenu *settingsMenu = menuBar()->addMenu("&Settings");
    QMenu *helpMenu = menuBar()->addMenu("&Help");

    QAction *openAction = new QAction("Open", this);
    QAction *saveAction = new QAction("Save", this);
    QAction *exitAction = new QAction("Exit", this);
    QAction *runAction = new QAction("Run",this);
    QAction *preferencesAction = new QAction("Preferences", this);
    QAction *aboutAction = new QAction("About", this);

    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    settingsMenu->addAction(preferencesAction);
    helpMenu->addAction(aboutAction);

    // === Create a top toolbar ===
    QToolBar *toolbar = addToolBar("Main Toolbar");
    toolbar->addAction(openAction);
    toolbar->addAction(saveAction);
    toolbar->addSeparator();
    //toolbar->addAction(preferencesAction);
    toolbar->addAction(runAction);

    // === Connect actions ===
    connect(openAction, &QAction::triggered, this, [=]() {
        QString fileName = QFileDialog::getOpenFileName(this, "Open Assembly File", "", "Assembly Files (*.s *.asm);;All Files (*)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                editor->setPlainText(file.readAll());
        }
    });

    connect(saveAction, &QAction::triggered, this, [=]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Assembly File", "", "Assembly Files (*.s *.asm);;All Files (*)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text))
                file.write(editor->toPlainText().toUtf8());
        }
    });

    connect(exitAction, &QAction::triggered, this, &MainWindow::close);

    connect(aboutAction, &QAction::triggered, this, [=]() {
        QMessageBox::about(this, "About", "RISC-V Visual Assembler\nBuilt with Qt");
    });

    connect(runAction,&QAction::triggered, this, [=](){
        QString code = editor->toPlainText();
        std::string asmcode = code.toStdString();

        std::string dissassmbledCode = assemble().
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
