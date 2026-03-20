#include "assembler/custom_pseudo_manager.h"
#include "globals.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
bool CustomPseudoManager::addCustomPseudoInstruction(const QString &pseudoInst, const QString &expansion, QString &errorMessage)
{
    // we will add the new pseudo instruction and its expansion to the custom pseudo instruction list
    // for now we will just return true to indicate that the pseudo instruction was added successfully
    // in future we will implement the actual logic to store and manage custom pseudo instructions
    ParsedPseudoInstruction newInstruction;
    if (!parse(newInstruction, pseudoInst, expansion, errorMessage)) 
    {
        return false;
    }
    if(!validatePseudoInstruction(newInstruction, errorMessage))
    {
        return false;
    }
    
    QJsonObject root = loadFromDisk(errorMessage);

    if(root.contains(newInstruction.name))
    {
        errorMessage = QString("Instruction \"%1\" already exists. Use update instead.").arg(newInstruction.name);
        return false;
    }

    QJsonObject instructionObject;
    instructionObject["args"] = QJsonArray::fromStringList(newInstruction.args);
    instructionObject["expansion"] = QJsonArray::fromStringList(newInstruction.expansion);
    root[newInstruction.name] = instructionObject;
    return saveToDisk(root, errorMessage);
}

bool CustomPseudoManager::parse(ParsedPseudoInstruction &parsedInst, const QString &pseudoInst, const QString &expansion, QString &errorMessage)
{
    
    const QStringList tokens = pseudoInst.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if(tokens.isEmpty())
    {
        errorMessage = "Pseudo instruction cannot be empty.";
        return false;
    }
    parsedInst.name = tokens.first();
    parsedInst.args = tokens.mid(1);
    for (const QString &line : expansion.split('\n', Qt::SkipEmptyParts)) 
    {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            parsedInst.expansion << trimmed;
    }

    return true;

}

bool CustomPseudoManager::validatePseudoInstruction(const ParsedPseudoInstruction &parsedInst, QString &errorMessage)
{
    static const QRegularExpression validNameRegex("^[a-zA-Z_][a-zA-Z0-9_.]*$");

    if(!validNameRegex.match(parsedInst.name).hasMatch())
    {
        errorMessage = QString("Invalid instruction name \"%1\". "
                               "Must start with a letter and contain only "
                               "letters, digits, underscores, or dots.").arg(parsedInst.name);
        return false;
    }
    

    //only register arg of the form r1,r1....r32 allowed
    static const QRegularExpression validArg(R"(^r([1-9]|[1-2][0-9]|3[0-2])$)");
    for (const QString &arg : parsedInst.args)
    {
        if (!validArg.match(arg).hasMatch())
        {
            errorMessage = QString("Invalid argument \"%1\". "
                                   "Must be a valid register name (e.g., r0, r1, ..., r32).").arg(arg);
            return false;
        }
    }

    for(const QString &line : parsedInst.expansion)
    {
        if(line.isEmpty())
            continue;
        // we will check if the line is a valid instruction format (for now we will just check if it starts with a valid instruction name)
        
        //hook tihis to the main parse there we will chekc if the intruction is valid or not
    }

    return true;
}


QJsonObject CustomPseudoManager::loadFromDisk(QString &errorMessage)
{
    QFile file(globals::custom_pseudo_instructions_file_path);
    if (!file.open(QIODevice::ReadOnly)) 
    {
        errorMessage = "Could not open file for reading.";
        return {};
    }

    
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if(parseError.error != QJsonParseError::NoError)
    {
        errorMessage = "Failed to parse JSON: " + parseError.errorString();
        return {};
    }

    return doc.object();
}

bool CustomPseudoManager::saveToDisk(const QJsonObject &root, QString &errorMessage)
{
    QFile file(globals::custom_pseudo_instructions_file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) 
    {
        errorMessage = "Could not open file for writing.";
        return false;
    }

    QJsonDocument doc(root);
    QByteArray data = doc.toJson(QJsonDocument::Indented);
    qint64 bytesWritten = file.write(data);
    file.close();
    if (bytesWritten != data.size()) 
    {
        errorMessage = "Failed to write all data to file.";
        return false;
    }

    return true;
}
