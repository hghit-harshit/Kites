#include "ui/editortab.h"
#include <QSplitter>
#include <QBoxLayout>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTextBlock>
#include <QTextEdit>
namespace Kites
{
EditorTab::EditorTab(QWidget* parent, VMManager* vmManager)
    : KitesTab(parent)
    , m_vmManager(vmManager)
{
    QSplitter* mainsplitter = new QSplitter(Qt::Horizontal, this);
     
    m_editor = new Editor(this);
    m_editor->setPlaceholderText("Enter your code here...");
    m_squiggleFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_squiggleFormat.setUnderlineColor(Qt::red);
    //m_editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    //m_editor->setStyleSheet("background-color:#0b0f14; color:#b8fbb8;");

    mainsplitter->addWidget(m_editor);

    m_disassemblyView = new Editor(this,false);
    m_disassemblyView->setReadOnly(true);
    //m_disassemblyView->setPlaceholderText("Disassembled code will appear here...");
    //m_disassemblyView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    //m_disassemblyView->setStyleSheet("background-color:#0b0f14; color:#b8fbb8;");
    mainsplitter->addWidget(m_disassemblyView);

    // m_registerContainer = new RegisterContainer(this);
    // mainsplitter->addWidget(m_registerContainer);

    //mainsplitter->setSizes({200, 100});
    // -----------------------

    // You also need to add the splitter to your tab's layout
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(mainsplitter);
    this->setLayout(layout);
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
