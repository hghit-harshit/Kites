#include "icon_manager.h"
#include "theme_manager.h"
#include <QPixmap>
#include <QPainter>
namespace Kites
{

IconManager::IconManager()
{
   setUpIconPaths();
}

IconManager &IconManager::getInstance()
{
    static IconManager instance;
    return instance;
}

QIcon IconManager::getIcon(Icon icon, QSize size) const
{
    const ThemeManager &themeManager = ThemeManager::getInstance();
    QColor iconColor = themeManager.getIconColor();

    QIcon baseIcon(m_iconPaths[toIndex(icon)]);
    QPixmap pixmap = baseIcon.pixmap(size);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), iconColor);
    painter.end();

    return QIcon(pixmap);
}

void IconManager::setUpIconPaths()
{
    m_iconPaths[toIndex(Icon::Play)]      = ":/icons/play.svg";
    m_iconPaths[toIndex(Icon::Stop)]      = ":/icons/stop.svg";
    m_iconPaths[toIndex(Icon::Pause)]     = ":/icons/pause.svg";
    m_iconPaths[toIndex(Icon::Resume)]    = ":/icons/resume.svg";
    m_iconPaths[toIndex(Icon::Step)]      = ":/icons/step.svg";
    m_iconPaths[toIndex(Icon::Undo)]      = ":/icons/undo-dot.svg";
    m_iconPaths[toIndex(Icon::Redo)]      = ":/icons/redo-dot.svg";

    // m_iconPaths[toIndex(Icon::Editor)]    = ":/icons/editor.svg";
    // m_iconPaths[toIndex(Icon::Memory)]    = ":/icons/memory.svg";
    // m_iconPaths[toIndex(Icon::Processor)] = ":/icons/processor.svg";
    // m_iconPaths[toIndex(Icon::Cache)]     = ":/icons/cache.svg";
    // m_iconPaths[toIndex(Icon::Settings)]  = ":/icons/settings.svg";
}
}//namespace Kites