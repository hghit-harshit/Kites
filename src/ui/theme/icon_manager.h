#pragma once
//#include <QObject>
#include <QIcon>
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
};
}//namespace Kites