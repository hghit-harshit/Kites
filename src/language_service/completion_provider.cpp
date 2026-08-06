#include "completion_provider.h"
#include "common/register_names.h"
#include "custom_pseudo_manager/custom_pseudo_manager.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace Kites
{
namespace
{
// Not present in instructions.json (which only covers mnemonics) - the assembler
// (src/assembler/parse_formats) recognizes these separately.
const QStringList &directiveList()
{
    static const QStringList directives = {".data", ".text",  ".bss",  ".word", ".half",
                                            ".byte", ".dword", ".asciz", ".ascii", ".string",
                                            ".globl", ".global", ".align", ".space", ".zero"};
    return directives;
}
} // namespace

CompletionProvider::CompletionProvider()
{
    loadMnemonicsAndDirectives();
    loadRegisters();
    refreshCustomPseudoInstructions(); // also rebuilds the cache
}

CompletionProvider &CompletionProvider::getInstance()
{
    static CompletionProvider instance;
    return instance;
}

void CompletionProvider::loadMnemonicsAndDirectives()
{
    QStringList mnemonics;

    QFile file(":/instructions/instructions.json");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        const QJsonObject root = doc.object();
        for (const QString &category : root.keys())
        {
            for (const QJsonValue &value : root.value(category).toArray())
            {
                mnemonics << value.toString();
            }
        }
    }

    mnemonics << directiveList();
    m_mnemonicsAndDirectives = mnemonics;
}

void CompletionProvider::loadRegisters()
{
    QStringList registers;
    for (const char *name : register_names::kGprNames)
        registers << name;
    for (const char *alias : register_names::kGprAliases)
        registers << alias;
    for (const char *name : register_names::kFprNames)
        registers << name;
    for (const char *alias : register_names::kFprAliases)
        registers << alias;
    m_registers = registers;
}

void CompletionProvider::refreshCustomPseudoInstructions()
{
    QString errorMessage;
    const QJsonObject pseudoInstructions =
        customPseudoManager::loadInstructionsFromDisk(errorMessage);
    m_customPseudoInstructions = pseudoInstructions.keys();
    rebuildCache();
}

void CompletionProvider::rebuildCache()
{
    QSet<QString> unique;
    for (const QString &symbol : m_mnemonicsAndDirectives)
        unique.insert(symbol);
    for (const QString &symbol : m_registers)
        unique.insert(symbol);
    for (const QString &symbol : m_customPseudoInstructions)
        unique.insert(symbol);

    m_cachedAll = QStringList(unique.begin(), unique.end());
    m_cachedAll.sort(Qt::CaseInsensitive);
}

const QStringList &CompletionProvider::staticSymbols() const
{
    return m_cachedAll;
}

} // namespace Kites
