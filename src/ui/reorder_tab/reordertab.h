#pragma once
#include "ui/common/kitestab.h"
#include <QCheckBox>
#include <QLabel>
#include <QPlainTextEdit>

namespace Kites
{
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
    void setReordered(const QString &text, int before, int after);
    void clear();

  private:
    QPlainTextEdit *m_view;
    QLabel *m_stats;
    QCheckBox *m_exec;
};
} // namespace Kites