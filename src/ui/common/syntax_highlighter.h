#pragma once
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include "ui/theme/theme_manager.h"
namespace Kites
{
class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
  public:
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);

  protected:
    void highlightBlock(const QString &text) override;
  public slots:
    void themeChangedSlot([[maybe_unused]] const QString &themeId);
    
  private:
    void setHighlightingRules();
    void setInstructionHighlightingRules();
    void setRegisterHighlightingRules();
    struct HighlightRule
    {
      QRegularExpression pattern;
      SyntaxStyle style;
    };

    QVector<HighlightRule> m_highlightRules;

    // format rules for comments
    //  QRegularExpression m_singleLineCommentPattern;
    //  QTextCharFormat m_singleLineCommentFormat;
};
} // namespace Kites
