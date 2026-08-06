/**
 * @file custom_pseudo_manager.h
 * @brief This files contains the declaration of the CustomPseudoManager class which is
 * responsible for managing custom pseudo instructions in the assembler.
 * @version 0.1
 * @date 2026-03-17
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <vector>

namespace Kites
{
namespace customPseudoManager
{
  
    bool addCustomPseudoInstruction(const QString &pseudoInst, const QString &expansion,
                                           QString &errorMessage, bool isUpdate = false);

    struct ParsedPseudoInstruction
    {
        QString name;
        QStringList args;
        QStringList expansion;
    };

    bool parse(ParsedPseudoInstruction &parsedInst, const QString &pseudoInst,
                      const QString &expansion, QString &errorMessage);
    bool validatePseudoInstruction(const ParsedPseudoInstruction &parsedInst,
                                          QString &errorMessage);
    bool isValidExpainsionLine(const QString &line, const QStringList &declaredArgs,
                                      QString &errorMessage);
    // bool validatePseudoInstructionFormat(const QString &expansion, QString &errorMessage);
    bool saveToDisk(const QJsonObject &root, QString &errorMessage);
    QJsonObject loadInstructionsFromDisk(QString &errorMessage);
    bool updateCustomPseudoInstruction(const QString &pseudoInst,
                                              const QString &newExpansion, QString &errorMessage);
    bool instructionExists(const QString &pseudoInst);
    /**
     * @brief Goes through the source code and replaces any occurrences of custom pseudo
     * instructions with their corresponding expansions.
     *
     * @param source
     * @return std::string
     */
    std::string expandPseudoInstruction(const std::string &source);

    /**
     * @brief Same as expandPseudoInstruction(), but also reports which original
     * source line each expanded line came from, since a single pseudo-instruction
     * can expand to multiple lines. Used to map diagnostics on expanded source
     * back to the line the user actually sees in the editor.
     *
     * @param source
     * @param expandedLineToOriginalLine Out param, 0-indexed by expanded line,
     *        value is the 1-based original source line number.
     * @return std::string
     */
    std::string expandPseudoInstruction(const std::string &source,
                                         std::vector<int> &expandedLineToOriginalLine);
}//namespace customPseudoManager
}//namespace Kites