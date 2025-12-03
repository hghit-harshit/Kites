#include "ui/syntax_highlighter.h"
#include <QFont>
namespace Kites
{
SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Define single line comment format
    // m_singleLineCommentPattern = QRegularExpression("#[^\n]*");
    // m_singleLineCommentFormat.setForeground(Qt::darkGreen);
    // m_singleLineCommentFormat.setFontItalic(true);

    // // Add the single line comment rule to highlight rules
    // HighlightRule singleLineCommentRule;
    // singleLineCommentRule.pattern = m_singleLineCommentPattern;
    // singleLineCommentRule.format = m_singleLineCommentFormat;
    // m_highlightRules.append(singleLineCommentRule);

    // QColor textColor = palette.text().color();
    // QColor keywordColor;
    // QColor labelColor;
    // QColor commentColor;

    // bool dark = (palette.color(QPalette::Window).lightness() < 128);

    // if (dark) 
    // {
    //     keywordColor = QColor("#569CD6");   // VSCode blue
    //     labelColor   = QColor("#C586C0");   // purple
    //     commentColor = QColor("#6A9955");   // green
    // } else 
    // {
    //     keywordColor = QColor("#0000AA");
    //     labelColor   = QColor("#800080");
    //     commentColor = QColor("#008000");
    // }
    setHighlightingRules();
    
}

void SyntaxHighlighter::setHighlightingRules()
{
    HighlightRule rule;

    QTextCharFormat instructionKeywordFormat;
    instructionKeywordFormat.setForeground(Qt::blue);
    instructionKeywordFormat.setFontWeight(QFont::Bold);

    QTextCharFormat registerFormat;
    registerFormat.setForeground(Qt::darkMagenta);


    const QStringList riscvInstructionPatterns = {
    // --- RV32I Base Instructions ---
    "\\blui\\b", "\\bauipc\\b", "\\bjal\\b", "\\bjalr\\b",
    "\\bbeq\\b", "\\bbne\\b", "\\bblt\\b", "\\bbge\\b",
    "\\bbltu\\b", "\\bbgeu\\b",
    "\\blb\\b", "\\blh\\b", "\\blw\\b", "\\blbu\\b", "\\blhu\\b",
    "\\bsb\\b", "\\bsh\\b", "\\bsw\\b",
    "\\baddi\\b", "\\bslti\\b", "\\bsltiu\\b",
    "\\bxori\\b", "\\bori\\b", "\\bandi\\b",
    "\\bslli\\b", "\\bsrli\\b", "\\bsrai\\b",
    "\\badd\\b", "\\bsub\\b", "\\bsll\\b", "\\bslt\\b",
    "\\bsltu\\b", "\\bxor\\b", "\\bsrl\\b", "\\bsra\\b",
    "\\bor\\b", "\\band\\b",
    "\\bfence\\b", "\\becall\\b", "\\bebreak\\b",
    "\\bnop\\b",

    // --- RV64I ---
    "\\blwu\\b", "\\bld\\b", "\\bsd\\b",
    "\\baddiw\\b", "\\bslliw\\b", "\\bsrliw\\b", "\\bsraiw\\b",
    "\\baddw\\b", "\\bsubw\\b", "\\bsllw\\b", "\\bsrlw\\b", "\\bsraw\\b",

    // --- M Extension (Multiply/Divide) ---
    "\\bmul\\b", "\\bmulh\\b", "\\bmulhsu\\b", "\\bmulhu\\b",
    "\\bdiv\\b", "\\bdivu\\b", "\\brem\\b", "\\bremu\\b",

    // --- A Extension (Atomics) ---
    "\\blr\\.w\\b", "\\bsc\\.w\\b", "\\bamoswap\\.w\\b",
    "\\bamoadd\\.w\\b", "\\bamoxor\\.w\\b", "\\bamoand\\.w\\b",
    "\\bamoorr\\.w\\b", "\\bamomin\\.w\\b", "\\bamomax\\.w\\b",
    "\\bamominu\\.w\\b", "\\bamomaxu\\.w\\b",
    "\\blr\\.d\\b", "\\bsc\\.d\\b", "\\bamoswap\\.d\\b",
    "\\bamoadd\\.d\\b", "\\bamoxor\\.d\\b", "\\bamoand\\.d\\b",
    "\\bamoorr\\.d\\b", "\\bamomin\\.d\\b", "\\bamomax\\.d\\b",
    "\\bamominu\\.d\\b", "\\bamomaxu\\.d\\b",

    // --- F Extension (Single-Precision Float) ---
    "\\bflw\\b", "\\bfsw\\b",
    "\\bfmadd\\.s\\b", "\\bfmsub\\.s\\b", "\\bfnmsub\\.s\\b", "\\bfnmadd\\.s\\b",
    "\\bfadd\\.s\\b", "\\bfsub\\.s\\b", "\\bfmul\\.s\\b", "\\bfdiv\\.s\\b",
    "\\bsqrt\\.s\\b",
    "\\bfsgnj\\.s\\b", "\\bfsgnjn\\.s\\b", "\\bfsgnjx\\.s\\b",
    "\\bmin\\.s\\b", "\\bmax\\.s\\b",
    "\\bcvt\\.w\\.s\\b", "\\bcvt\\.wu\\.s\\b",
    "\\bmv\\.x\\.s\\b", "\\bclass\\.s\\b",
    "\\beq\\.s\\b", "\\bne\\.s\\b", "\\blt\\.s\\b", "\\bge\\.s\\b",
    "\\blt\\.u\\.s\\b", "\\bge\\.u\\.s\\b",

    // --- D Extension (Double-Precision Float) ---
    "\\bfld\\b", "\\bfsd\\b",
    "\\bfmadd\\.d\\b", "\\bfmsub\\.d\\b", "\\bfnmsub\\.d\\b", "\\bfnmadd\\.d\\b",
    "\\bfadd\\.d\\b", "\\bfsub\\.d\\b", "\\bfmul\\.d\\b", "\\bfdiv\\.d\\b",
    "\\bsqrt\\.d\\b",
    "\\bfsgnj\\.d\\b", "\\bfsgnjn\\.d\\b", "\\bfsgnjx\\.d\\b",
    "\\bmin\\.d\\b", "\\bmax\\.d\\b",
    "\\bcvt\\.s\\.d\\b", "\\bcvt\\.d\\.s\\b",
    "\\bcvt\\.w\\.d\\b", "\\bcvt\\.wu\\.d\\b",
    "\\bcvt\\.d\\.w\\b", "\\bcvt\\.d\\.wu\\b",
    "\\bclass\\.d\\b",
    "\\beq\\.d\\b", "\\bne\\.d\\b", "\\blt\\.d\\b", "\\bge\\.d\\b",

    };

    const QStringList riscvRegisterPatterns = {
    // --- Numeric registers ---
    "\\bx0\\b",  "\\bx1\\b",  "\\bx2\\b",  "\\bx3\\b",
    "\\bx4\\b",  "\\bx5\\b",  "\\bx6\\b",  "\\bx7\\b",
    "\\bx8\\b",  "\\bx9\\b",  "\\bx10\\b", "\\bx11\\b",
    "\\bx12\\b", "\\bx13\\b", "\\bx14\\b", "\\bx15\\b",
    "\\bx16\\b", "\\bx17\\b", "\\bx18\\b", "\\bx19\\b",
    "\\bx20\\b", "\\bx21\\b", "\\bx22\\b", "\\bx23\\b",
    "\\bx24\\b", "\\bx25\\b", "\\bx26\\b", "\\bx27\\b",
    "\\bx28\\b", "\\bx29\\b", "\\bx30\\b", "\\bx31\\b",

    // --- ABI names ---
    "\\bzero\\b", "\\bra\\b",  "\\bsp\\b",  "\\bgp\\b", "\\btp\\b",
    "\\bt0\\b",   "\\bt1\\b",  "\\bt2\\b",
    "\\bs0\\b",   "\\bs1\\b",
    "\\ba0\\b",   "\\ba1\\b",  "\\ba2\\b",  "\\ba3\\b",
    "\\ba4\\b",   "\\ba5\\b",  "\\ba6\\b",  "\\ba7\\b",
    "\\bs2\\b",   "\\bs3\\b",  "\\bs4\\b",  "\\bs5\\b",
    "\\bs6\\b",   "\\bs7\\b",  "\\bs8\\b",  "\\bs9\\b",
    "\\bs10\\b",  "\\bs11\\b",
    "\\bt3\\b",   "\\bt4\\b",  "\\bt5\\b",  "\\bt6\\b",

    // --- Floating-point registers (F extension) ---
    "\\bf0\\b",  "\\bf1\\b",  "\\bf2\\b",  "\\bf3\\b",  "\\bf4\\b",  "\\bf5\\b",  "\\bf6\\b",  "\\bf7\\b",
    "\\bf8\\b",  "\\bf9\\b",  "\\bf10\\b", "\\bf11\\b", "\\bf12\\b", "\\bf13\\b", "\\bf14\\b", "\\bf15\\b",
    "\\bf16\\b", "\\bf17\\b", "\\bf18\\b", "\\bf19\\b", "\\bf20\\b", "\\bf21\\b", "\\bf22\\b", "\\bf23\\b",
    "\\bf24\\b", "\\bf25\\b", "\\bf26\\b", "\\bf27\\b", "\\bf28\\b", "\\bf29\\b", "\\bf30\\b", "\\bf31\\b"
    };


    for(const QString &pattern : riscvInstructionPatterns) 
    {
        rule.pattern = QRegularExpression(pattern);
        rule.format = instructionKeywordFormat;
        m_highlightRules.append(rule);
    }

    for(const QString &pattern : riscvRegisterPatterns) 
    {
        rule.pattern = QRegularExpression(pattern);
        rule.format = registerFormat;
        m_highlightRules.append(rule);
    }

    // setting rules for comments
    m_highlightRules.append({
        QRegularExpression("#[^\n]*"),
        [](){
            QTextCharFormat format;
            format.setForeground(Qt::gray);
            return format;
        }()
    });

    // setting rules for labels
    m_highlightRules.append({
        QRegularExpression(R"(^\s*[A-Za-z_][A-Za-z0-9_.$]*:$)"),
        [](){
            QTextCharFormat format;
            format.setForeground(QColor(128,0,128));
            return format;
        }()
    });
    //for number literals
    m_highlightRules.append({
        QRegularExpression(R"(\b0[xX][0-9a-fA-F]+|\b\d+|\b0[bB][01]+)"),
        [](){
            QTextCharFormat format;
            format.setForeground(Qt::darkGreen);
            return format;
        }()
    });
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightRule &rule : m_highlightRules) 
    {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) 
        {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}


void SyntaxHighlighter::setTheme(bool isDarkMode)
{
    // Clear existing rules
    m_highlightRules.clear();

    
}// namespace Kites
}