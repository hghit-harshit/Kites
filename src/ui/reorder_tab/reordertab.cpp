#include "reordertab.h"
#include "reorder_editor.h"
#include <QVBoxLayout>

namespace Kites
{
ReorderTab::ReorderTab(QWidget *parent) : KitesTab(parent)
{
    m_view = new ReorderEditor(this);
    m_stats = new QLabel("No program loaded", this);

    m_exec = new QCheckBox("Execute reordered code on next Run", this);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_exec);
    layout->addWidget(m_stats);
    layout->addWidget(m_view);
}

void ReorderTab::setReordered(const QString &text, const std::vector<int> &origIndex, int before,
                              int after)
{
    m_view->setReordered(text, origIndex);
    m_stats->setText(
        QString("Cycles: %1 -> %2 (saved %3)").arg(before).arg(after).arg(before - after));
}

void ReorderTab::clear()
{
    m_view->clearReordered();
    m_stats->setText("Reordering not available for this processor");
}
} // namespace Kites
