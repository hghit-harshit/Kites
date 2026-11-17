#pragma once
#include "ui/common/kitestab.h"
#include <QCheckBox>
#include <QLabel>
#include <vector>

namespace Kites
{
class ReorderEditor;

class ReorderTab : public KitesTab
{
    Q_OBJECT
  public:
    explicit ReorderTab(QWidget *parent = nullptr);
    bool executeReordered() const
    {
        return m_exec->isChecked();
    }

  public slots:
    // text: one line per instruction; origIndex[k] = original instruction # of line k
    void setReordered(const QString &text, const std::vector<int> &origIndex, int before,
                      int after);
    void clear();

  private:
    ReorderEditor *m_view;
    QLabel *m_stats;
    QCheckBox *m_exec;
};
} // namespace Kites
