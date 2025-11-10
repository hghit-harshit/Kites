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
namespace Kites
{

namespace Ui{
class EditorTab;
}
class EditorTab : public KitesTab
{

    
    public:
        explicit EditorTab(QWidget* parent = nullptr);
        void updateDisassemblyView(const std::string& disassembledCode);
        void setErrorLinesFromFile(const std::filesystem::path& filepath);
        std::string getRawText();
        void resetErrorLines();
    
    private:
        Editor* m_editor = nullptr;
        QPlainTextEdit* m_disassemblyView = nullptr;
        QTextCharFormat m_squiggleFormat; // stores how the quiggles will look
        //std::map<int, QString> m_errorMessages; // line number (1-based) to error message
        //std::list<int> m_errorLines;
        //RegisterContainer* m_registerContainer = nullptr;
    public slots:
        void highlightLine(int lineNumber);
};
} // namespace Kites
