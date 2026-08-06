#include "syntax_highlighter.h"
#include <QFont>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>

namespace Kites
{
namespace
{
    std::map<std::string, SyntaxStyle> stringToSyntaxStyle = {
    #define X(enumValue, stringValue) {stringValue, SyntaxStyle::enumValue},
        ENUM_FROM_STRING_LIST
        //magic!!
    #undef X
    };

}

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    setHighlightingRules();
    connect(&ThemeManager::getInstance(), &ThemeManager::editorThemeChangedSignal,
    this, &SyntaxHighlighter::themeChangedSlot);
}

void SyntaxHighlighter::setHighlightingRules()
{
    
    setInstructionHighlightingRules();
    setRegisterHighlightingRules();

    // setting rules for comments
    m_highlightRules.append({QRegularExpression("#[^\n]*"), SyntaxStyle::Comment});

    // setting rules for labels
    m_highlightRules.append({QRegularExpression(R"(^\s*[A-Za-z_][A-Za-z0-9_.$]*:$)"), SyntaxStyle::Label});
    // for number literals
    m_highlightRules.append({QRegularExpression(R"((?<!\w)(?:-?(?:0x[0-9A-Fa-f]+|0b[01]+|\d+))\b)"), 
                            SyntaxStyle::NumberLiteral});
}

void SyntaxHighlighter::setInstructionHighlightingRules()
{
    QFile instructionFile(":/instructions/instructions.json");
    // yeah this instructions/instructions.json
    //is ugly but my OCD wil not allow me to put 
    // it outside of a folder
    if (!instructionFile.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open instruction JSON file:" << instructionFile.errorString();
        return;
    }

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(instructionFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << "Failed to parse instruction JSON:" << parseError.errorString();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    for (auto it = jsonObj.begin(); it != jsonObj.end(); ++it)
    {
        const QString category = it.key();
        const QJsonArray instructions = it.value().toArray();

        QStringList instructionNames;

        for (const auto& value : instructions)
        {
            instructionNames << QRegularExpression::escape(
                                    value.toString());
        }

        if (instructionNames.isEmpty())
        {
            continue;
        }

        QString regexPattern =
            "\\b(" + instructionNames.join("|") + ")\\b";

        HighlightRule rule;

        rule.pattern = QRegularExpression(regexPattern);

        rule.style = stringToSyntaxStyle[category.toStdString()];

        m_highlightRules.push_back(std::move(rule));
    }
}

void SyntaxHighlighter::setRegisterHighlightingRules()
{
    const QString registerPattern {
        "\\b(?:"
        "x(?:[0-9]|[12][0-9]|3[01])|"
        "f(?:[0-9]|[12][0-9]|3[01])|"
        "zero|ra|sp|gp|tp|"
        "t[0-6]|"
        "s(?:[0-9]|1[01])|"
        "a[0-7]"
        ")\\b"
    };

    HighlightRule rule;
    rule.pattern = QRegularExpression(registerPattern);
    rule.style = SyntaxStyle::Register;
    m_highlightRules.push_back(std::move(rule));
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightRule &rule : m_highlightRules)
    {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext())
        {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), 
            ThemeManager::getInstance().getSyntaxFormat(rule.style));
        }
    }
}

void SyntaxHighlighter::themeChangedSlot([[maybe_unused]] const QString &themeId)
{
    rehighlight();
}

} // namespace Kites
