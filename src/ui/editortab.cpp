#include "ui/editortab.h"
#include "ui_editortab.h"
#include "assembler/custom_pseudo_manager.h"
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>

namespace Kites
{

EditorTab::EditorTab(QWidget* parent, VMManager* vmManager)
    : KitesTab(parent)
    , m_vmManager(vmManager)
    , ui(new Ui::EditorTab)
{
    ui->setupUi(this);
    ui->editorViewButton->setChecked(true); // default to editor view
    ui->stackedWidget->setCurrentIndex(0); // show editor view by default   

    ui->stackedWidget->layout()->setContentsMargins(0, 0, 0, 0);
    ui->stackedWidget->layout()->setSpacing(0);
    ui->stackedWidget->widget(0)->layout()->setContentsMargins(0, 0, 0, 0);
    ui->stackedWidget->widget(1)->layout()->setContentsMargins(0, 0, 0, 0);

    m_editor = ui->assemblyTextEdit;
    m_disassemblyView = ui->disassemblyTextEdit;
    m_expandedView = ui->expandedTextEdit;
    m_expandedView->setReadOnly(true);
    m_editor->setPlaceholderText("Enter your code here...");
    m_squiggleFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_squiggleFormat.setUnderlineColor(Qt::red);

    connect(ui->expandedViewButton, &QRadioButton::toggled, this, &EditorTab::onExpandButtonClicked);
}

void EditorTab::onExpandButtonClicked(bool checked)
{
    if(checked)
    {
        std::string expandedCode = CustomPseudoManager::expandPseudoInstruction(getRawText());
        m_expandedView->setPlainText(QString::fromStdString(expandedCode));
        ui->stackedWidget->setCurrentIndex(1);
    }
    else
    {
        ui->stackedWidget->setCurrentIndex(0);
    }
}

void EditorTab::switchToExpandedView()
{
    ui->expandedViewButton->setChecked(true);
}

EditorTab::~EditorTab()
{
    delete ui;
}

std::string EditorTab::getRawText()
{
    return m_editor->toPlainText().toStdString();
}

std::vector<uint64_t> EditorTab::getBreakpoints() const
{
    return m_editor->getBreakpoints();
}

void EditorTab::setRawText(const QString& text)
{
    m_editor->setPlainText(text);
}

void EditorTab::resetErrorLines()
{
    m_editor->setExtraSelections({});
    m_editor->resetErrors();
}

void EditorTab::updateDisassemblyView(const std::string& disassembledCode)
{
    m_disassemblyView->setPlainText(QString::fromStdString(disassembledCode));
}

void EditorTab::highlightLines(const QVariantMap& editorLines, const QVariantMap& disassemblyLines)
{
    m_editor->setLinesToHighlight(editorLines);
    m_disassemblyView->setLinesToHighlight(disassemblyLines);
}

void EditorTab::setCanWrite(bool canWrite)
{
    m_editor->setReadOnly(!canWrite);
}

void EditorTab::clearHighlights()
{
    m_editor->clearHighlights();
    m_disassemblyView->clearHighlights();
}

void EditorTab::setErrorLinesFromFile(const std::filesystem::path& filePath)
{
    // This will hold all the squiggles we want to draw
    QList<QTextEdit::ExtraSelection> extraSelections;
    //m_errorMessages.clear();
    // --- 1. Read and Parse the JSON File ---
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) 
    {
        qWarning() << "Could not open error file:" << filePath.string();
        return;
    }

    QByteArray fileData = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(fileData);

    if (doc.isNull() || !doc.isObject()) 
    {
        qWarning() << "Failed to parse error JSON.";
        return;
    }

    QJsonObject rootObject = doc.object();
    QJsonArray errors = rootObject["errors"].toArray();

    // --- 2. Build the ExtraSelection List ---
    for (const QJsonValue &value : errors)
    {
        QJsonObject errorObj = value.toObject();
        
        int line = errorObj["line"].toInt();
        QString message = errorObj["message"].toString();

        if (line <= 0) continue; // Skip invalid lines

        QTextBlock block = m_editor->document()->findBlockByNumber(line - 1); // -1 since line is 1 based
        if (!block.isValid()) {
            continue; // Line number is out of bounds
        }
        QTextEdit::ExtraSelection selection;
        selection.format = m_squiggleFormat;
        qDebug() << "Setting tooltip for line" << errorObj["line"].toInt() << ":" << message;
        m_editor->setErrorMessage(line-1,message); // passing error to editor for tooltip display
        //m_errorMessages[line] = message;
        //selection.format.setToolTip(message);
        //selection.format.setToolTip("THIS IS A DRILL");
        QTextCursor cursor(block);
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        selection.cursor = cursor;
        extraSelections.append(selection);
    }

    // --- 4. Apply all squiggles at once ---
    m_editor->setExtraSelections(extraSelections);
}
} // namespace Kites
