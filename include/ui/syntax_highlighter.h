#pragma once
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>

namespace Kites
{
class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    public:
        explicit SyntaxHighlighter(QTextDocument *parent = nullptr);
    protected:
        void highlightBlock(const QString &text) override;
        void setTheme(bool isDarkMode);
    private:
        void setHighlightingRules();
        struct HighlightRule
        {
            QRegularExpression pattern;
            QTextCharFormat format;
        };

        QVector<HighlightRule> m_highlightRules;
    
        //format rules for comments
        // QRegularExpression m_singleLineCommentPattern;
        // QTextCharFormat m_singleLineCommentFormat;
        
};
}// namespace Kites
