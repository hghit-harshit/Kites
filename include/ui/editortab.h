#pragma once 
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

namespace Ui{
class EditorTab;
}
class EditorTab : public KitesTab
{
    Q_OBJECT    
    public:
        explicit EditorTab(QWidget* parent = nullptr,VMManager *vmManager = nullptr);
        void updateDisassemblyView(const std::string& disassembledCode);
        void setErrorLinesFromFile(const std::filesystem::path& filepath);
        std::string getRawText();
        void setRawText(const QString& text);
        void resetErrorLines();
        void highlightLines(const QVariantMap& editorLines, const QVariantMap& disassemblyLines);
        void setCanWrite(bool canWrite);
        std::vector<uint64_t> getBreakpoints() const;
    private:
        Editor* m_editor = nullptr;
        Editor* m_disassemblyView = nullptr;
        // m_disassemblyView is an object of the same class as m_editor
        // but well make it read only and use it to show disassembled code
        // were doing this cause of the overriden paint event in Editor class to show hightlights
        QTextCharFormat m_squiggleFormat; // stores how the quiggles will look
        VMManager* m_vmManager = nullptr;
        //std::map<int, QString> m_errorMessages; // line number (1-based) to error message
        //std::list<int> m_errorLines;
        //RegisterContainer* m_registerContainer = nullptr;
    //public slots:
        
};
} // namespace Kites
