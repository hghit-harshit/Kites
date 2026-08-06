#pragma once
#include <QStringList>

namespace Kites
{

/**
 * @brief Supplies the static (buffer-independent) part of autocomplete: RV32/64
 * mnemonics, pseudo-instructions, assembler directives, and register names/aliases,
 * plus any user-defined custom pseudo-instructions. Live symbols (labels) are the
 * caller's (LanguageService's) responsibility since they depend on buffer content.
 */
class CompletionProvider
{
  public:
    static CompletionProvider &getInstance();

    // All static completion candidates (mnemonics, directives, registers, custom
    // pseudo-instructions). Cheap to call repeatedly - computed once and cached.
    const QStringList &staticSymbols() const;

    // Re-reads custom pseudo-instructions from disk (call after the user edits
    // them in Settings) and refreshes the cached symbol list.
    void refreshCustomPseudoInstructions();

  private:
    CompletionProvider();

    void loadMnemonicsAndDirectives();
    void loadRegisters();
    void rebuildCache();

    QStringList m_mnemonicsAndDirectives;
    QStringList m_registers;
    QStringList m_customPseudoInstructions;
    QStringList m_cachedAll;
};

} // namespace Kites
