#pragma once
#include <QWidget>

class QToolButton;
class QParallelAnimationGroup;

namespace Kites
{

/**
 * @brief CollapsibleSection is a reusable header + body container that
 * animates open/closed, similar to a Zed sidebar section.
 */
class CollapsibleSection : public QWidget
{
    Q_OBJECT
  public:
    explicit CollapsibleSection(const QString &title, QWidget *parent = nullptr);

    // Places `content` inside the section's body. Ownership is transferred.
    void setContentWidget(QWidget *content);
    void setExpanded(bool expanded, bool animate = true);
    bool isExpanded() const;

  private:
    QToolButton *m_headerButton;
    QWidget *m_bodyWidget;
    QWidget *m_contentWidget{nullptr};
    QParallelAnimationGroup *m_animation;
};

} // namespace Kites
