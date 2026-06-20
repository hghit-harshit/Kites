#include "theme_manager.h"
#include <QFile>
#include <QTextStream>
#include <QApplication>
namespace Kites
{

ThemeManager::ThemeManager() : m_currentThemeType(ThemeType::Dark)
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
    QApplication::setPalette(m_themes[static_cast<size_t>(theme)].palette);

    qApp->setStyleSheet(loadStyleSheet(m_themes[static_cast<size_t>(theme)].stylesheetPath));
    emit themeChangedSignal(theme);
}

QColor ThemeManager::getIconColor() const
{
    return m_themes[static_cast<size_t>(m_currentThemeType)].iconColor;
}

QColor ThemeManager::getTextColor() const
{
    return m_themes[static_cast<size_t>(m_currentThemeType)].textColor;
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
        ThemeData &theme = m_themes[static_cast<size_t>(ThemeType::Light)];
        theme.type           = ThemeType::Light;
        theme.name           = "Light";
        theme.stylesheetPath = ":/themes/aqua_light.qss";
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
    }
    {
        ThemeData &theme = m_themes[static_cast<size_t>(ThemeType::Dark)];
        theme.type           = ThemeType::Dark;
        theme.name           = "Dark";
        theme.stylesheetPath = ":/themes/elegant_dark.qss";
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

        // p.setColor(QPalette::Window, QColor("#FFFFFF"));
        // p.setColor(QPalette::WindowText, QColor("#1A1A1A"));
        // p.setColor(QPalette::Base, QColor("#F5F5F5"));
        // p.setColor(QPalette::AlternateBase, QColor("#FFFFFF"));
        // p.setColor(QPalette::ToolTipBase, QColor("#FFFFFF"));
        // p.setColor(QPalette::ToolTipText, QColor("#1A1A1A"));
        // p.setColor(QPalette::Text, QColor("#1A1A1A"));
        // p.setColor(QPalette::Button, QColor("#F0F0F0"));
        // p.setColor(QPalette::ButtonText, QColor("#1A1A1A"));
        // p.setColor(QPalette::Highlight, QColor("#2A82DA"));
        // p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
        // p.setColor(QPalette::Link, QColor("#2A82DA"));

        // // Disabled group
        // p.setColor(QPalette::Disabled, QPalette::Window, QColor("#ECECEC"));
        // p.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#969696"));
        // p.setColor(QPalette::Disabled, QPalette::Base, QColor("#ECECEC"));
        // p.setColor(QPalette::Disabled, QPalette::Text, QColor("#969696"));
        // p.setColor(QPalette::Disabled, QPalette::Button, QColor("#DCDCDC"));
        // p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#969696"));

        // p.setColor(QPalette::Inactive, QPalette::Highlight, Qt::transparent);
        // p.setColor(QPalette::Inactive, QPalette::HighlightedText, Qt::black);
    }
}

}//namespace Kites