#include "ui/compilertab.h"
#include "ui_compilertab.h"
#include <QProcess>
#include <QTemporaryFile>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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

    QString escaped = cppSource;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    escaped.replace("\n", "\\n");
    escaped.replace("\r", "\\r");
    escaped.replace("\t", "\\t");

    QString jsonBody = QString(R"({
        "source": "%1",
        "options": {
            "userArguments": "-O0 -march=rv64gc -mabi=lp64d -fno-exceptions",
            "filters": {
                "binary":      false,
                "directives":  true,
                "commentOnly": true,
                "trim":        true,
                "labels":      false,
                "intel":       false
            },
            "compilerOptions": {}
        }
    })").arg(escaped);

    QUrl url("https://godbolt.org/api/compiler/rv64-gcc1520/compile");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    

    ui->outputTextEdit->setPlainText("Compiling via Godbolt API...");
    ui->convertToAssemblyButton->setEnabled(false); // prevent double-clicks
    // 2. Write to temp file

    // 5. Send POST request
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkReply* reply = manager->post(request, jsonBody.toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        // Re-enable button
        ui->convertToAssemblyButton->setEnabled(true);

        // Check for network-level errors (no internet, timeout, etc.)
        if (reply->error() != QNetworkReply::NoError) {
            ui->outputTextEdit->setPlainText(
                "Network error: " + reply->errorString() + 
                "\n\nMake sure you have an internet connection."
            );
            reply->deleteLater();
            manager->deleteLater();
            return;
        }

        // 7. Parse JSON response
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);

        if (doc.isNull() || !doc.isObject()) {
            ui->outputTextEdit->setPlainText("Error: Invalid response from Godbolt API.");
            reply->deleteLater();
            manager->deleteLater();
            return;
        }

        QJsonObject root = doc.object();

        // 8. Check compiler exit code
        int exitCode = root["code"].toInt(-1);

        // 9. Collect stderr (compiler warnings/errors)
        QString stderrOutput;
        QJsonArray stderrLines = root["stderr"].toArray();
        for (const QJsonValue& line : stderrLines)
            stderrOutput += line.toObject()["text"].toString() + "\n";

        // If compilation failed, show errors
        if (exitCode != 0) {
            ui->outputTextEdit->setPlainText(
                "Compilation failed (exit code " + QString::number(exitCode) + "):\n\n" +
                stderrOutput
            );
            reply->deleteLater();
            manager->deleteLater();
            return;
        }

        // 10. Extract assembly lines from "asm" array
        QString asmOutput;
        QJsonArray asmLines = root["asm"].toArray();

        if (asmLines.isEmpty()) {
            ui->outputTextEdit->setPlainText("Error: No assembly output returned.");
            reply->deleteLater();
            manager->deleteLater();
            return;
        }

        for (const QJsonValue& line : asmLines) {
            QString text = line.toObject()["text"].toString();
            asmOutput += text + "\n";
        }

        // 11. Show warnings (if any) above the assembly
        QString finalOutput;
        if (!stderrOutput.trimmed().isEmpty())
            finalOutput += "// Warnings:\n// " + 
                           stderrOutput.trimmed().replace("\n", "\n// ") + 
                           "\n\n";
        finalOutput += asmOutput;

        ui->outputTextEdit->setPlainText(finalOutput);

        reply->deleteLater();
        manager->deleteLater();
    });


}

CompilerTab::~CompilerTab()
{
    delete ui;
}
}// namespce Kites
