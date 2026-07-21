#ifndef EDITORTAB__H
#define EDITORTAB__H

#include "main_editor.h"
#include "disassembly_editor.h"
#include "processor/processor_manager.h"
#include "ui/common/kitestab.h"
#include <QPlainTextEdit>
#include <QString>
#include <QTextCharFormat>
#include <filesystem>

namespace Kites
{
namespace Ui
{
class EditorTab;
}

class EditorTab : public KitesTab
{
    Q_OBJECT

  public:
    explicit EditorTab(QWidget *parent = nullptr, ProcessorManager *vmManager = nullptr);
    ~EditorTab();
    void updateDisassemblyView(const std::string &disassembledCode);
    void setErrorLinesFromFile(const std::filesystem::path &filepath);
    std::string getRawText();
    void setRawText(const QString &text);
    void resetErrorLines();
    void highlightLines(const std::vector<std::pair<int,std::string>> &editorLines, 
      const std::vector<std::pair<int,std::string>> &disassemblyLines);
    void setCanWrite(bool canWrite);
    void setExpandedLocked(bool locked);
    std::vector<uint64_t> getBreakpoints() const;
    void clearHighlights();

  public slots:
    void onExpandButtonClicked(bool checked);
    void switchToExpandedView();

  private:
    Editor *m_editor = nullptr;
    DisassemblyEditor *m_disassemblyView = nullptr;
    Editor *m_expandedView = nullptr;
    // m_disassemblyView is an object of the same class as m_editor
    // but well make it read only and use it to show disassembled code
    // were doing this cause of the overriden paint event in Editor class to show hightlights
    QTextCharFormat m_squiggleFormat; // stores how the quiggles will look
    ProcessorManager *m_processorManager = nullptr;
    QTextCursor m_userCursorPosition;
    // to save user cursor position when updating disassembly view
    bool m_expandedLocked = false;
    // public slots:
    Ui::EditorTab *ui;
};
} // namespace Kites
#endif EDITORTAB__H
