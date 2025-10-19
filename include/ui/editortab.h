#pragma once 
#include "kitestab.h"
#include <QPlainTextEdit>
#include <QFontDatabase>
#include <QString>
#include "registercontainer.h"

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
        std::string getRawText();
    private:
        QPlainTextEdit* m_editor = nullptr;
        QPlainTextEdit* m_disassemblyView = nullptr;
        //RegisterContainer* m_registerContainer = nullptr;
};
} // namespace Kites
