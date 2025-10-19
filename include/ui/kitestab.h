#pragma once
#include <QWidget>
#include <QToolBar>

namespace Kites
{
class KitesTab : public QWidget
{
    public:
    KitesTab(QWidget* parent = nullptr);
    QToolBar* getToolbar() const { return m_toolbar; }
    protected:
        QToolBar* m_toolbar = nullptr;
};
} // namespace Kites
