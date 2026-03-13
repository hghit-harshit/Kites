#include "ui/compilertab.h"
#include "ui_compilertab.h"
#include <QProcess>
#include <QTemporaryFile>
#include <QFile>
#include <QTextStream>
#include <QDir>

namespace Kites
{

CompilerTab::CompilerTab(QWidget *parent)
    : KitesTab(parent)
    , ui(new Ui::CompilerTab)
{
    ui->setupUi(this);
    connect(ui->convertToAssemblyButton, &QPushButton::clicked, this, &CompilerTab::convertToAssemblyClicked);
}

void CompilerTab::convertToAssemblyClicked()
{
     // 1. Read C++ from input text edit
    QString cppSource = ui->inputTextEdit->toPlainText();

    if (cppSource.trimmed().isEmpty()) {
        ui->outputTextEdit->setPlainText("Error: Input is empty.");
        return;
    }

    // 2. Write to temp file
    QTemporaryFile tmpCpp(QDir::tempPath() + "/kites_input_XXXXXX.cpp");
    if (!tmpCpp.open()) {
        ui->outputTextEdit->setPlainText("Error: Could not create temp file.");
        return;
    }

    QTextStream ts(&tmpCpp);
    ts << cppSource;
    ts.flush();

    QString cppFilePath = tmpCpp.fileName();
    QString asmFilePath = cppFilePath + ".s";

    // 3. Choose compiler
    QString compiler;
#ifdef Q_OS_WIN
    compiler = "riscv64-unknown-elf-g++.exe";  // or clang++.exe
#else
    compiler = "riscv64-unknown-elf-g++";      // or clang++
#endif

    // 4. Arguments
    QStringList args;
    args << "-S"
         << "-march=rv64gc"
         << cppFilePath
         << "-o" << asmFilePath;

    // 5. Run compiler
    QProcess proc;
    proc.start(compiler, args);
    if (!proc.waitForFinished()) {
        ui->outputTextEdit->setPlainText("Error: Compiler did not finish.");
        return;
    }

    // 6. Check errors
    QString err = proc.readAllStandardError();
    if (!err.isEmpty()) {
        ui->outputTextEdit->setPlainText("Compiler error:\n" + err);
        return;
    }

    // 7. Read assembly
    QFile asmFile(asmFilePath);
    if (!asmFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->outputTextEdit->setPlainText("Error: Could not open assembly output.");
        return;
    }

    QString asmText = asmFile.readAll();

    // 8. Show in output text edit
    ui->outputTextEdit->setPlainText(asmText);
}

CompilerTab::~CompilerTab()
{
    delete ui;
}
}// namespce Kites
