#include "collapsible_section.h"
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QToolButton>
#include <QVBoxLayout>

namespace Kites
{
namespace
{
constexpr int animationDurationMs = 150;
}

CollapsibleSection::CollapsibleSection(const QString &title, QWidget *parent)
    : QWidget(parent), m_headerButton(new QToolButton(this)), m_bodyWidget(new QWidget(this)),
      m_animation(new QParallelAnimationGroup(this))
{
    m_headerButton->setObjectName("collapsibleSectionHeader");
    m_headerButton->setText(title);
    m_headerButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_headerButton->setArrowType(Qt::DownArrow);
    m_headerButton->setCheckable(true);
    m_headerButton->setChecked(true);
    m_headerButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_bodyWidget->setMaximumHeight(0);
    m_bodyWidget->setMinimumHeight(0);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_headerButton);
    mainLayout->addWidget(m_bodyWidget);

    m_animation->addAnimation(new QPropertyAnimation(m_bodyWidget, "maximumHeight"));

    connect(m_animation, &QParallelAnimationGroup::finished, this,
            [this]()
            {
                if (m_headerButton->isChecked())
                {
                    // Fully open: let the body grow with the layout instead of
                    // staying pinned to the sizeHint captured at animation start,
                    // and let this section itself compete for/absorb extra space.
                    m_bodyWidget->setMaximumHeight(QWIDGETSIZE_MAX);
                    setMaximumHeight(QWIDGETSIZE_MAX);
                    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
                }
                else
                {
                    // Fully collapsed: stop competing for leftover space in the
                    // parent layout so sibling sections take it instead.
                    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                    setMaximumHeight(m_headerButton->sizeHint().height());
                }
                updateGeometry();
            });

    connect(m_headerButton, &QToolButton::toggled, this,
            [this](bool checked) { setExpanded(checked); });

    setExpanded(true, /*animate=*/false);
}

void CollapsibleSection::setContentWidget(QWidget *content)
{
    if (m_contentWidget)
    {
        m_bodyWidget->layout()->removeWidget(m_contentWidget);
        m_contentWidget->deleteLater();
    }

    m_contentWidget = content;

    auto *bodyLayout = m_bodyWidget->layout();
    if (!bodyLayout)
    {
        bodyLayout = new QVBoxLayout(m_bodyWidget);
        bodyLayout->setContentsMargins(0, 0, 0, 0);
    }
    bodyLayout->addWidget(content);

    setExpanded(isExpanded(), /*animate=*/false);
}

void CollapsibleSection::setExpanded(bool expanded, bool animate)
{
    m_headerButton->setChecked(expanded);
    m_headerButton->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);

    const int targetHeight = expanded && m_contentWidget ? m_contentWidget->sizeHint().height() : 0;

    m_animation->stop();
    auto *heightAnimation = qobject_cast<QPropertyAnimation *>(m_animation->animationAt(0));
    heightAnimation->setDuration(animate ? animationDurationMs : 0);
    heightAnimation->setStartValue(m_bodyWidget->maximumHeight());
    heightAnimation->setEndValue(targetHeight);
    m_animation->start();
}

bool CollapsibleSection::isExpanded() const
{
    return m_headerButton->isChecked();
}

} // namespace Kites
