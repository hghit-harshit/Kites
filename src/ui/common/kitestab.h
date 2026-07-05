#pragma once
#include <QToolBar>
#include <QWidget>

namespace Kites
{
class KitesTab : public QWidget
{
  public:
    KitesTab(QWidget *parent = nullptr);
  protected:
    QToolBar *m_toolbar = nullptr;
};
} // namespace Kites
