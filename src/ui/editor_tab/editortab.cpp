#include "editortab.h"
#include "custom_pseudo_manager/custom_pseudo_manager.h"
#include "ui/theme/theme_manager.h"
#include "ui_editortab.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace Kites
{

EditorTab::EditorTab(QWidget *parent, ProcessorManager *vmManager)
    : KitesTab(parent), m_processorManager(vmManager), ui(new Ui::EditorTab)
{
    ui->setupUi(this);
    ui->editorViewButton->setChecked(true); // default to editor view
    ui->stackedWidget->setCurrentIndex(0);  // show editor view by default

    ui->stackedWidget->layout()->setContentsMargins(0, 0, 0, 0);
    ui->stackedWidget->layout()->setSpacing(0);
    ui->stackedWidget->widget(0)->layout()->setContentsMargins(0, 0, 0, 0);
    ui->stackedWidget->widget(1)->layout()->setContentsMargins(0, 0, 0, 0);

    m_editor = ui->assemblyTextEdit;
    m_disassemblyView = ui->disassemblyTextEdit;
    m_expandedView = ui->expandedTextEdit;
    m_expandedView->setReadOnly(true);
    m_expandedView->setBreakpointInteractionEnabled(false);
    m_expandedView->setBreakpoints(m_editor->getBreakpoints());
    m_editor->setPlaceholderText("Enter your code here...");
    m_squiggleFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_squiggleFormat.setUnderlineColor(ThemeManager::getInstance().getEditorErrorColor());
    connect(&ThemeManager::getInstance(), &ThemeManager::editorThemeChangedSignal, this,
            [this](const QString &)
            {
                m_squiggleFormat.setUnderlineColor(
                    ThemeManager::getInstance().getEditorErrorColor());
            });

    connect(ui->expandedViewButton, &QRadioButton::toggled, this,
            &EditorTab::onExpandButtonClicked);

    m_languageService = new LanguageService(this);
    connect(m_languageService, &LanguageService::diagnosticsReadySignal, this,
            [this](const QVector<Diagnostic> &diagnostics)
            {
                if (m_editor->isReadOnly())
                    return; // a Run started while this request was in flight; Run owns the
                            // squiggles now, don't clobber them with a stale live result
                applyDiagnostics(diagnostics);
            });

    m_diagnosticsDebounceTimer = new QTimer(this);
    m_diagnosticsDebounceTimer->setSingleShot(true);
    m_diagnosticsDebounceTimer->setInterval(300);
    connect(m_diagnosticsDebounceTimer, &QTimer::timeout, this,
            &EditorTab::requestLiveDiagnostics);
    connect(m_editor, &QPlainTextEdit::textChanged, this,
            [this]()
            {
                m_diagnosticsDebounceTimer->start();
                emit contentChangedSignal();
            });
}

void EditorTab::requestLiveDiagnostics()
{
    if (m_editor->isReadOnly())
        return; // running/debugging - Run's own diagnostics own the squiggles right now
    m_languageService->requestDiagnostics(QString::fromStdString(getRawText()));
}

void EditorTab::onExpandButtonClicked(bool checked)
{
    if (m_expandedLocked && !checked)
    {
        ui->expandedViewButton->setChecked(true);
        return;
    }

    if (checked)
    {
        std::string expandedCode = customPseudoManager::expandPseudoInstruction(getRawText());
        m_expandedView->setPlainText(QString::fromStdString(expandedCode));
        m_expandedView->setBreakpoints(m_editor->getBreakpoints());
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

void EditorTab::switchToEditorView()
{
    ui->editorViewButton->setChecked(true);
}

void EditorTab::setExpandedLocked(bool locked)
{
    m_expandedLocked = locked;
    ui->editorViewButton->setEnabled(!locked);

    if (locked)
    {
        ui->expandedViewButton->setChecked(true);
        ui->stackedWidget->setCurrentIndex(1);
    }
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

void EditorTab::setRawText(const QString &text)
{
    m_editor->setPlainText(text);
}

void EditorTab::resetErrorLines()
{
    m_editor->setExtraSelections({});
    m_editor->resetErrors();
}

void EditorTab::updateDisassemblyView(const std::string &disassembledCode)
{
    m_disassemblyView->setPlainText(QString::fromStdString(disassembledCode));
}

void EditorTab::highlightLines(const std::vector<std::pair<int,std::string>> &editorLines, 
const std::vector<std::pair<int,std::string>> &disassemblyLines)
{
    m_editor->setLinesToHighlight(editorLines);
    m_disassemblyView->setLinesToHighlight(disassemblyLines);
    m_expandedView->setLinesToHighlight(
        editorLines); // we want the expanded view to have the same highlights as the editor
}

void EditorTab::setCanWrite(bool canWrite)
{
    m_editor->setReadOnly(!canWrite);
}

void EditorTab::showRuntimeError(int line, const QString &message)
{
    if (line <= 0)
        return;
    m_editor->setLinesToHighlight({{line, "Runtime Error"}});
    m_editor->setErrorMessage(line - 1, message);
}

void EditorTab::clearHighlights()
{
    m_editor->clearHighlights();
    m_expandedView->clearHighlights();
    m_disassemblyView->clearHighlights();
}

void EditorTab::setErrorLinesFromFile(const std::filesystem::path &filePath)
{
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

    QVector<Diagnostic> diagnostics;
    diagnostics.reserve(errors.size());
    for (const QJsonValue &value : errors)
    {
        QJsonObject errorObj = value.toObject();
        diagnostics.append(
            Diagnostic{errorObj["line"].toInt(), 0, errorObj["message"].toString()});
    }

    applyDiagnostics(diagnostics);
}

void EditorTab::applyDiagnostics(const QVector<Diagnostic> &diagnostics)
{
    m_editor->resetErrors();

    QList<QTextEdit::ExtraSelection> extraSelections;
    for (const Diagnostic &diagnostic : diagnostics)
    {
        if (diagnostic.line <= 0)
            continue; // Skip invalid lines

        QTextBlock block = m_editor->document()->findBlockByNumber(
            diagnostic.line - 1); // -1 since line is 1 based
        if (!block.isValid())
        {
            continue; // Line number is out of bounds
        }
        QTextEdit::ExtraSelection selection;
        selection.format = m_squiggleFormat;
        m_editor->setErrorMessage(diagnostic.line - 1,
                                  diagnostic.message); // passing error to editor for tooltip display

        QTextCursor cursor(block);
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        selection.cursor = cursor;
        extraSelections.append(selection);
    }

    m_editor->setExtraSelections(extraSelections);
    m_expandedView->setExtraSelections(extraSelections);
}
} // namespace Kites
