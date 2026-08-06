#pragma once
//#include <QObject>
#include <QIcon>
#include <QHash>
#include <QPixmap>
#include <array>
#include "utils/to_index.h"
namespace Kites
{
enum class Icon
{
    Play,
    Stop,
    Pause,
    Resume,
    Step,
    Undo,
    Redo,
    Editor,
    Memory,
    Processor,
    Cache,
    Compiler,
    Profiler,
    Open,
    Save,
    Settings,
    About,
    IconCount
};

class IconManager //: QObject
{
    //Q_OBJECT
public:
    static IconManager &getInstance();
    ~IconManager() = default;
    QIcon getIcon(Icon icon, QSize size = QSize(18, 18)) const;
private:
    IconManager();
    // using size for futue proofing this fucntion.
    void setUpIconPaths();
    std::array<QString, toIndex(Icon::IconCount)> m_iconPaths;
    mutable QHash<QString, QPixmap> m_pixmapCache; // keyed by "icon:size:color"
};
}//namespace Kites
