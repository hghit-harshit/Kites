#ifndef EDITORTAB__H
#define EDITORTAB__H

#include "kitestab.h"
#include <QPlainTextEdit>
#include <QFontDatabase>
#include <QString>
#include "registercontainer.h"
#include <list>
#include <filesystem>
#include <QTextCharFormat>
#include "editor.h"
#include "vm/vm_manager.h"
namespace Kites
{
namespace Ui {
class EditorTab;
}

class EditorTab : public KitesTab
{
    Q_OBJECT

 public:
        explicit EditorTab(QWidget* parent = nullptr,VMManager *vmManager = nullptr);
        ~EditorTab();
        void updateDisassemblyView(const std::string& disassembledCode);
        void setErrorLinesFromFile(const std::filesystem::path& filepath);
        std::string getRawText();
        void setRawText(const QString& text);
        void resetErrorLines();
        void highlightLines(const QVariantMap& editorLines, const QVariantMap& disassemblyLines);
        void setCanWrite(bool canWrite);
        void setExpandedLocked(bool locked);
        std::vector<uint64_t> getBreakpoints() const;
        //void saveUserCursorPosition();
        void clearHighlights();
        
    public slots:
        void onExpandButtonClicked(bool checked);
        void switchToExpandedView();
    private:
        Editor* m_editor = nullptr;
        Editor* m_disassemblyView = nullptr;
        Editor* m_expandedView = nullptr;
        // m_disassemblyView is an object of the same class as m_editor
        // but well make it read only and use it to show disassembled code
        // were doing this cause of the overriden paint event in Editor class to show hightlights
        QTextCharFormat m_squiggleFormat; // stores how the quiggles will look
        VMManager* m_vmManager = nullptr;
        QTextCursor m_userCursorPosition;
        // to save user cursor position when updating disassembly view
        bool m_expandedLocked = false;
    //public slots:
        Ui::EditorTab *ui;
};
} // namespace Kites
#endif // EDITORTAB__H
