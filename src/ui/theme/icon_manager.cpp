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

    const QString cacheKey = QString("%1:%2x%3:%4")
                                  .arg(toIndex(icon))
                                  .arg(size.width())
                                  .arg(size.height())
                                  .arg(iconColor.name());

    auto cached = m_pixmapCache.constFind(cacheKey);
    if (cached != m_pixmapCache.constEnd())
        return QIcon(cached.value());

    QIcon baseIcon(m_iconPaths[toIndex(icon)]);
    QPixmap pixmap = baseIcon.pixmap(size);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), iconColor);
    painter.end();

    m_pixmapCache.insert(cacheKey, pixmap);
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

    m_iconPaths[toIndex(Icon::Editor)]    = ":/icons/editor.svg";
    m_iconPaths[toIndex(Icon::Memory)]    = ":/icons/memory.svg";
    m_iconPaths[toIndex(Icon::Processor)] = ":/icons/processor.svg";
    m_iconPaths[toIndex(Icon::Cache)]     = ":/icons/cache.svg";
    m_iconPaths[toIndex(Icon::Compiler)]  = ":/icons/compiler.svg";
    m_iconPaths[toIndex(Icon::Profiler)]  = ":/icons/profiler.svg";
    m_iconPaths[toIndex(Icon::Open)]      = ":/icons/open.svg";
    m_iconPaths[toIndex(Icon::Save)]      = ":/icons/save.svg";
    m_iconPaths[toIndex(Icon::Settings)]  = ":/icons/settings.svg";
    m_iconPaths[toIndex(Icon::About)]     = ":/icons/about.svg";
}
}//namespace Kites
