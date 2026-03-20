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
#include <QString>
#include <QJsonObject>
#include <QStringList>
/**
 * @brief This class is responsible for managing custom pseudo instructions in the assembler. 
 * It allows users to add new pseudo instructions and their expansions, which can then be used in assembly code. 
 * The class will handle the storage and retrieval of these custom pseudo instructions, as well as any necessary validation.
 * 
 */
class CustomPseudoManager
{
public:
    static bool addCustomPseudoInstruction(const QString &pseudoInst, const QString &expansion,QString &errorMessage);
   
    
    
private:

    struct ParsedPseudoInstruction
    {
        QString name;
        QStringList args;
        QStringList expansion;
    };
    
    static bool parse(ParsedPseudoInstruction &parsedInst, const QString &pseudoInst, const QString &expansion, QString &errorMessage);
    static bool validatePseudoInstruction(const ParsedPseudoInstruction &parsedInst, QString &errorMessage);
    //bool validatePseudoInstructionFormat(const QString &expansion, QString &errorMessage);
    static bool saveToDisk(const QJsonObject &root, QString &errorMessage);
    static QJsonObject loadFromDisk(QString &errorMessage);
    static bool updateCustomPseudoInstruction(const QString &pseudoInst, const QString &newExpansion, QString &errorMessage);
    static bool instructionExists(const QString &pseudoInst);


};