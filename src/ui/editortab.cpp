#include "ui/editortab.h"
#include <QSplitter>
#include <QBoxLayout>
namespace Kites
{
EditorTab::EditorTab(QWidget* parent)
    : KitesTab(parent)
{
    QSplitter* mainsplitter = new QSplitter(Qt::Horizontal, this);

    m_editor = new QPlainTextEdit(this);
    m_editor->setPlaceholderText("Enter your code here...");
    //m_editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    //m_editor->setStyleSheet("background-color:#0b0f14; color:#b8fbb8;");

    mainsplitter->addWidget(m_editor);

    m_disassemblyView = new QPlainTextEdit(this);
    m_disassemblyView->setReadOnly(true);
    m_disassemblyView->setPlaceholderText("Disassembled code will appear here...");
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

void EditorTab::updateDisassemblyView(const std::string& disassembledCode)
{
    m_disassemblyView->setPlainText(QString::fromStdString(disassembledCode));
}
} // namespace Kites
