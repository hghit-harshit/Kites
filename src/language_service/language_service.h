#pragma once
#include "diagnostic.h"
#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QVector>

namespace Kites
{

/**
 * @brief In-process "language service" for the assembly editor: live diagnostics
 * (real assembler errors, not regex) and autocomplete candidates. No external
 * process, no JSON-RPC - see docs/plan for why (no reliable RISC-V LSP server
 * to depend on, and Kites already owns a real assembler).
 */
class LanguageService : public QObject
{
    Q_OBJECT
  public:
    explicit LanguageService(QObject *parent = nullptr);

    // Kicks off a background assemble of `source` and emits diagnosticsReadySignal
    // when done. Safe to call again before a previous request finishes - only the
    // latest request's result is ever emitted (QFutureWatcher supersedes it).
    void requestDiagnostics(const QString &source);

    // Synchronous and cheap: merges static symbols (mnemonics/directives/registers/
    // custom pseudo-instructions) with labels found in `source`, filtered to those
    // starting with `prefix` (case-insensitive). Empty prefix returns no results -
    // callers should only invoke this once the user has typed something. Doesn't
    // depend on instance state (CompletionProvider is a singleton, label scanning
    // is pure), so callers that only need completions don't need a LanguageService.
    static QStringList completions(const QString &prefix, const QString &source);

  signals:
    void diagnosticsReadySignal(const QVector<Diagnostic> &diagnostics);

  private slots:
    void handleAssembleFinished();

  private:
    QFutureWatcher<QVector<Diagnostic>> m_watcher;
};

} // namespace Kites
