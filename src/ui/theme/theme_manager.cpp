#include "theme_manager.h"
#include <QFile>
#include <QTextStream>
#include <QApplication>
namespace Kites
{

ThemeManager::ThemeManager()
{
    setUpThemes();
}

ThemeManager &ThemeManager::getInstance()
{
    static ThemeManager instance;
    return instance;
}

ThemeType ThemeManager::currentThemeType() const
{
    return m_currentThemeType;
}

void ThemeManager::setTheme(ThemeType theme)
{
    if (theme == m_currentThemeType)
        return;

    m_currentThemeType = theme;
    QApplication::setPalette(m_themes[toIndex(theme)].palette);
    if (!m_themes[toIndex(theme)].stylesheetPath.isEmpty())
        qApp->setStyleSheet(loadStyleSheet(m_themes[toIndex(theme)].stylesheetPath));
    emit themeChangedSignal(theme);
}

QColor ThemeManager::getIconColor() const
{
    return m_themes[toIndex(m_currentThemeType)].iconColor;
}

QColor ThemeManager::getTextColor() const
{
    return m_themes[toIndex(m_currentThemeType)].textColor;
}

QTextCharFormat ThemeManager::getSyntaxFormat(SyntaxStyle style) const
{
    return m_themes[toIndex(m_currentThemeType)].syntaxFormats[static_cast<int>(style)];
}

QString ThemeManager::loadStyleSheet(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "ThemeManager: failed to load stylesheet:" << path;
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

void ThemeManager::setUpThemes()
{
    /**TODO:
     * We will remove these palette and we will
     * depend only on qss to get the colors and styles for the widgets.
     */
    {
        ThemeData &theme = m_themes[toIndex(ThemeType::Light)];
        theme.type           = ThemeType::Light;
        theme.name           = "Light";
        theme.stylesheetPath = "";
        theme.iconColor      = QColor("#1A1A1A");
        theme.textColor      = QColor("#1A1A1A");
        theme.cacheHitColor  = QColor("#2ada5c");
        theme.cacheMissColor = QColor("#DA2A2A");

        QPalette &p = theme.palette;

        // Active / normal groups
        p.setColor(QPalette::Window,          QColor("#EFEFF4"));
        p.setColor(QPalette::WindowText,      QColor("#1A1A1A"));
        p.setColor(QPalette::Base,            QColor("#FFFFFF"));
        p.setColor(QPalette::AlternateBase,   QColor("#F5F5F5"));
        p.setColor(QPalette::ToolTipBase,     QColor("#FFFFFF"));
        p.setColor(QPalette::ToolTipText,     QColor("#1A1A1A"));
        p.setColor(QPalette::Text,            QColor("#1A1A1A"));
        p.setColor(QPalette::Button,          QColor("#EEEEEE"));
        p.setColor(QPalette::ButtonText,      QColor("#1A1A1A"));
        p.setColor(QPalette::Highlight,       QColor("#2A82DA"));
        p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
        p.setColor(QPalette::Link,            QColor("#2A82DA"));

        // Disabled group (Muted / Faded options)
        p.setColor(QPalette::Disabled, QPalette::Window,     QColor("#F5F5F5"));
        p.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#8E8E93"));
        p.setColor(QPalette::Disabled, QPalette::Base,       QColor("#FAFAFA"));
        p.setColor(QPalette::Disabled, QPalette::Text,       QColor("#8E8E93"));
        p.setColor(QPalette::Disabled, QPalette::Button,     QColor("#F5F5F5"));
        p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#8E8E93"));

        auto& syntaxFormats = theme.syntaxFormats;
        syntaxFormats[static_cast<int>(SyntaxStyle::R32Instruction)].setForeground(QColor("#0000FF"));  // Blue
        syntaxFormats[static_cast<int>(SyntaxStyle::R32Instruction)].setFontWeight(QFont::Bold);
        syntaxFormats[static_cast<int>(SyntaxStyle::R64Instruction)].setForeground(QColor("#00FFFF"));  // Cyan
        syntaxFormats[static_cast<int>(SyntaxStyle::R64Instruction)].setFontWeight(QFont::Bold);
        syntaxFormats[static_cast<int>(SyntaxStyle::FInstruction)].setForeground(QColor("#FF00FF"));  // Magenta
        syntaxFormats[static_cast<int>(SyntaxStyle::FInstruction)].setFontWeight(QFont::Bold);
        syntaxFormats[static_cast<int>(SyntaxStyle::DInstruction)].setForeground(QColor("#FFFF00"));  // Yellow
        syntaxFormats[static_cast<int>(SyntaxStyle::DInstruction)].setFontWeight(QFont::Bold);
        syntaxFormats[static_cast<int>(SyntaxStyle::Register)].setForeground(QColor("#800080"));
        syntaxFormats[static_cast<int>(SyntaxStyle::Comment)].setForeground(QColor("#008000"));  // Green
        syntaxFormats[static_cast<int>(SyntaxStyle::Label)].setForeground(QColor("#FF4500"));  // OrangeRed
        syntaxFormats[static_cast<int>(SyntaxStyle::NumberLiteral)].setForeground(QColor("#FF0000"));  // Red
        syntaxFormats[static_cast<int>(SyntaxStyle::PseudoInstruction)].setForeground(QColor("#FF8C00"));  // DarkOrange
    }
    {
        ThemeData &theme = m_themes[toIndex(ThemeType::Dark)];
        theme.type           = ThemeType::Dark;
        theme.name           = "Dark";
        theme.stylesheetPath = "";
        theme.iconColor      = QColor("#FFFFFF");
        theme.textColor      = QColor("#FFFFFF");
        theme.cacheHitColor  = QColor("#2ADA5C");
        theme.cacheMissColor = QColor("#DA2A2A");

        QPalette &p = theme.palette;

        // Active / normal groups
        p.setColor(QPalette::Window,          QColor("#1A1A1A"));
        p.setColor(QPalette::WindowText,      QColor("#FFFFFF"));
        p.setColor(QPalette::Base,            QColor("#2A2A2A"));
        p.setColor(QPalette::AlternateBase,   QColor("#1A1A1A"));
        p.setColor(QPalette::ToolTipBase,     QColor("#2A2A2A"));
        p.setColor(QPalette::ToolTipText,     QColor("#FFFFFF"));
        p.setColor(QPalette::Text,            QColor("#FFFFFF"));
        p.setColor(QPalette::Button,          QColor("#2A2A2A"));
        p.setColor(QPalette::ButtonText,      QColor("#FFFFFF"));
        p.setColor(QPalette::Highlight,       QColor("#2A82DA"));
        p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
        p.setColor(QPalette::Link,            QColor("#2A82DA"));

        // Disabled group (Dimmed against dark background)
        p.setColor(QPalette::Disabled, QPalette::Window,     QColor("#0D0D0D"));
        p.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#808080"));
        p.setColor(QPalette::Disabled, QPalette::Base,       QColor("#151515"));
        p.setColor(QPalette::Disabled, QPalette::Text,       QColor("#808080"));
        p.setColor(QPalette::Disabled, QPalette::Button,     QColor("#151515"));
        p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#808080"));

        auto& syntaxFormats = theme.syntaxFormats;
        syntaxFormats[static_cast<int>(SyntaxStyle::R32Instruction)].setForeground(QColor("#569CD6")); // Blue
        syntaxFormats[static_cast<int>(SyntaxStyle::R32Instruction)].setFontWeight(QFont::Bold);
        syntaxFormats[static_cast<int>(SyntaxStyle::R64Instruction)].setForeground(QColor("#4FC1FF")); // Light Cyan
        syntaxFormats[static_cast<int>(SyntaxStyle::R64Instruction)].setFontWeight(QFont::Bold);
        syntaxFormats[static_cast<int>(SyntaxStyle::FInstruction)].setForeground(QColor("#C586C0")); // Purple
        syntaxFormats[static_cast<int>(SyntaxStyle::FInstruction)].setFontWeight(QFont::Bold);
        syntaxFormats[static_cast<int>(SyntaxStyle::DInstruction)].setForeground(QColor("#DCDCAA")); // Soft Yellow
        syntaxFormats[static_cast<int>(SyntaxStyle::DInstruction)].setFontWeight(QFont::Bold);
        syntaxFormats[static_cast<int>(SyntaxStyle::Register)].setForeground(QColor("#9CDCFE")); // Light Blue
        syntaxFormats[static_cast<int>(SyntaxStyle::Comment)].setForeground(QColor("#6A9955")); // Green
        syntaxFormats[static_cast<int>(SyntaxStyle::Label)].setForeground(QColor("#D7BA7D")); // Gold
        syntaxFormats[static_cast<int>(SyntaxStyle::NumberLiteral)].setForeground(QColor("#B5CEA8")); // Pale Green
        syntaxFormats[static_cast<int>(SyntaxStyle::PseudoInstruction)].setForeground(QColor("#FF8C00")); // Dark Orange
    }
}

}//namespace Kites
