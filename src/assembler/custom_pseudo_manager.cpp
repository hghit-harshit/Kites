#include "assembler/custom_pseudo_manager.h"
#include "globals.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

bool CustomPseudoManager::addCustomPseudoInstruction(const QString &pseudoInst,
                                                     const QString &expansion,
                                                     QString &errorMessage, bool isUpdate)
{
    // we will add the new pseudo instruction and its expansion to the custom pseudo instruction
    // list for now we will just return true to indicate that the pseudo instruction was added
    // successfully in future we will implement the actual logic to store and manage custom pseudo
    // instructions
    ParsedPseudoInstruction newInstruction;
    if (!parse(newInstruction, pseudoInst, expansion, errorMessage))
    {
        return false;
    }
    if (!validatePseudoInstruction(newInstruction, errorMessage))
    {
        return false;
    }

    QJsonObject root = loadInstructionsFromDisk(errorMessage);

    if (!isUpdate && root.contains(newInstruction.name))
    {
        errorMessage = QString("Instruction \"%1\" already exists. Use update instead.")
                           .arg(newInstruction.name);
        return false;
    }
    else if (isUpdate)
    {
        // remove the old instruction and add the updated one
        root.remove(newInstruction.name);
    }

    QJsonObject instructionObject;
    instructionObject["args"] = QJsonArray::fromStringList(newInstruction.args);
    instructionObject["expansion"] = QJsonArray::fromStringList(newInstruction.expansion);
    root[newInstruction.name] = instructionObject;
    return saveToDisk(root, errorMessage);
}

bool CustomPseudoManager::parse(ParsedPseudoInstruction &parsedInst, const QString &pseudoInst,
                                const QString &expansion, QString &errorMessage)
{

    const QStringList tokens = pseudoInst.split(QRegularExpression("[\\s,]"), Qt::SkipEmptyParts);
    if (tokens.isEmpty())
    {
        errorMessage = "Pseudo instruction cannot be empty.";
        return false;
    }
    parsedInst.name = tokens.first();

    // Accept arguments separated by spaces and/or commas (commas may
    // optionally be followed by a space). Example accepted forms:
    //   "r1 r2"  "r1, r2"  "r1,r2"  "r1, r2,r3"
    QStringList args;
    const QStringList rawArgs = tokens.mid(1);
    for (const QString &raw : rawArgs)
    {
        const QStringList pieces = raw.split(',', Qt::KeepEmptyParts);
        for (QString p : pieces)
        {
            p = p.trimmed();
            if (p.isEmpty())
            {
                errorMessage = "Invalid pseudo instruction: empty argument (consecutive commas?)";
                return false;
            }
            args << p;
        }
    }
    parsedInst.args = args;

    for (const QString &line : expansion.split('\n', Qt::SkipEmptyParts))
    {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;

        // Normalize expansion line: accept commas between operands with or
        // without spaces and store a whitespace-separated normalized form.
        QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        QStringList normalizedParts;
        for (const QString &part : parts)
        {
            const QStringList sub = part.split(',', Qt::KeepEmptyParts);
            for (QString s : sub)
            {
                s = s.trimmed();
                if (s.isEmpty())
                {
                    errorMessage = "Invalid expansion: empty operand (consecutive commas?)";
                    return false;
                }
                normalizedParts << s;
            }
        }
        parsedInst.expansion << normalizedParts.join(' ');
    }

    return true;
}

bool CustomPseudoManager::validatePseudoInstruction(const ParsedPseudoInstruction &parsedInst,
                                                    QString &errorMessage)
{
    static const QRegularExpression validNameRegex("^[a-zA-Z_][a-zA-Z0-9_.]*$");

    if (!validNameRegex.match(parsedInst.name).hasMatch())
    {
        errorMessage = QString("Invalid instruction name \"%1\". "
                               "Must start with a letter and contain only "
                               "letters, digits, underscores, or dots.")
                           .arg(parsedInst.name);
        return false;
    }

    // only register arg of the form r1,r1....r32 allowed
    static const QRegularExpression validArg(R"(^r([1-9]|[1-2][0-9]|3[0-2])$)");
    for (const QString &arg : parsedInst.args)
    {
        if (!validArg.match(arg).hasMatch())
        {
            errorMessage = QString("Invalid argument \"%1\". "
                                   "Must be a valid register name (e.g., r0, r1, ..., r32).")
                               .arg(arg);
            return false;
        }
    }

    for (const QString &line : parsedInst.expansion)
    {
        if (line.isEmpty())
            continue;
        if (!isValidExpainsionLine(line, parsedInst.args, errorMessage))
        {
            return false;
        }
    }

    return true;
}

std::string CustomPseudoManager::expandPseudoInstruction(const std::string &source)
{
    QString errorMessage; // just a dummy variable
    QJsonObject pseudoInstrcutions = loadInstructionsFromDisk(errorMessage);

    if (pseudoInstrcutions.isEmpty())
    {
        return source;
    }

    const QString qSource = QString::fromStdString(source);
    const QStringList sourceLines = qSource.split('\n');
    QStringList expandedLines;

    for (const QString &line : sourceLines)
    {
        const QString trimmedLine = line.trimmed();

        if (trimmedLine.isEmpty())
        {
            expandedLines << line;
            continue;
        }

        const QStringList tokens =
            trimmedLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        const QString instName = tokens.first();

        if (!pseudoInstrcutions.contains(instName))
        {
            expandedLines << line;
            continue;
        }

        const QJsonObject instructionObject = pseudoInstrcutions[instName].toObject();
        const QJsonArray argsArray = instructionObject["args"].toArray();
        const QJsonArray expansionArray = instructionObject["expansion"].toArray();

        QStringList declaredArgs;

        // Accept arguments separated by spaces and/or commas (commas may
        // optionally be present without a following space). Reject empty
        // pieces (consecutive commas) by treating as mismatch.
        bool badArgs = false;
        for (int i = 1; i < tokens.size(); i++)
        {
            const QStringList pieces = tokens[i].split(',', Qt::KeepEmptyParts);
            for (QString p : pieces)
            {
                p = p.trimmed();
                if (p.isEmpty())
                {
                    badArgs = true;
                    break;
                }
                declaredArgs << p;
            }
            if (badArgs)
                break;
        }

        if (declaredArgs.size() != argsArray.size())
        {
            // argument count mismatch, treat as normal instruction
            expandedLines << line;
            continue;
        }

        QHash<QString, QString> argMapping;
        for (int i = 0; i < argsArray.size(); i++)
        {
            argMapping[argsArray[i].toString()] = declaredArgs[i];
        }

        for (const QJsonValue &expansionLineval : expansionArray)
        {
                // Split expansion line on whitespace, then accept commas between
                // operands; detect empty pieces (consecutive commas) and treat
                // as invalid expansion (skip expansion).
                QStringList rawTokens = expansionLineval.toString().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                QStringList expansionLineTokens;
                bool badExpansion = false;
                for (const QString &rt : rawTokens)
                {
                    const QStringList subs = rt.split(',', Qt::KeepEmptyParts);
                    for (QString s : subs)
                    {
                        s = s.trimmed();
                        if (s.isEmpty())
                        {
                            badExpansion = true;
                            break;
                        }
                        expansionLineTokens << s;
                    }
                    if (badExpansion)
                        break;
                }
                if (badExpansion)
                {
                    // malformed expansion template; fall back to leaving source line as-is
                    expandedLines << line;
                    break;
                }

            for (QString &token : expansionLineTokens)
            {
                if (argMapping.contains(token))
                {
                    token = argMapping[token];
                }
            }

            if (expansionLineTokens.size() <= 1)
            {
                expandedLines << expansionLineTokens.join(' ');
            }
            else
            {
                const QString mnemonic = expansionLineTokens.first();
                const QStringList operands = expansionLineTokens.mid(1);
                expandedLines << QString("%1 %2").arg(mnemonic, operands.join(", "));
            }
        }
    }

    return expandedLines.join('\n').toStdString();
}

bool CustomPseudoManager::isValidExpainsionLine(const QString &line,
                                                const QStringList &declaredArgs,
                                                QString &errorMessage)
{
    static std::unordered_map<QString, std::pair<int, std::set<int>>> instruction_to_operand_count =
        {{"add", {3, {}}},
         {"sub", {3, {}}},
         {"and", {3, {}}},
         {"or", {3, {}}},
         {"xor", {3, {}}},
         {"sll", {3, {}}},
         {"srl", {3, {}}},
         {"sra", {3, {}}},
         {"slt", {3, {}}},
         {"sltu", {3, {}}},
         {"addw", {3, {}}},
         {"subw", {3, {}}},
         {"sllw", {3, {}}},
         {"srlw", {3, {}}},
         {"sraw", {3, {}}},

         // I-type: rd, rs1, imm
         {"addi", {3, {2}}},
         {"xori", {3, {2}}},
         {"ori", {3, {2}}},
         {"andi", {3, {2}}},
         {"slli", {3, {2}}},
         {"srli", {3, {2}}},
         {"srai", {3, {2}}},
         {"slti", {3, {2}}},
         {"sltiu", {3, {2}}},
         {"addiw", {3, {2}}},
         {"slliw", {3, {2}}},
         {"srliw", {3, {2}}},
         {"sraiw", {3, {2}}},

         // Load: rd, imm(rs1) — treated as rd, rs1, imm
         {"lb", {3, {2}}},
         {"lh", {3, {2}}},
         {"lw", {3, {2}}},
         {"ld", {3, {2}}},
         {"lbu", {3, {2}}},
         {"lhu", {3, {2}}},
         {"lwu", {3, {2}}},

         // Store: rs2, imm(rs1) — treated as rs1, rs2, imm
         {"sb", {3, {2}}},
         {"sh", {3, {2}}},
         {"sw", {3, {2}}},
         {"sd", {3, {2}}},

         // Branch: rs1, rs2, imm
         {"beq", {3, {2}}},
         {"bne", {3, {2}}},
         {"blt", {3, {2}}},
         {"bge", {3, {2}}},
         {"bltu", {3, {2}}},
         {"bgeu", {3, {2}}},

         // U-type: rd, imm
         {"lui", {2, {1}}},
         {"auipc", {2, {1}}},

         // Jump
         {"jal", {2, {1}}},
         {"jalr", {3, {2}}},

         // No operands
         {"ecall", {0, {}}},
         {"ebreak", {0, {}}},
         {"nop", {0, {}}},
         {"ret", {0, {}}},
         {"fence", {0, {}}},
         {"fence_i", {0, {}}},

         // CSR: rd, csr, rs1
         {"csrrw", {3, {1}}},
         {"csrrs", {3, {1}}},
         {"csrrc", {3, {1}}},
         {"csrrwi", {3, {1, 2}}},
         {"csrrsi", {3, {1, 2}}},
         {"csrrci", {3, {1, 2}}},

         // Pseudos with operands
         {"la", {2, {1}}},
         {"li", {2, {1}}},
         {"mv", {2, {}}},
         {"not", {2, {}}},
         {"neg", {2, {}}},
         {"negw", {2, {}}},
         {"sext.w", {2, {}}},
         {"seqz", {2, {}}},
         {"snez", {2, {}}},
         {"sltz", {2, {}}},
         {"sgtz", {2, {}}},

         // Branch pseudos: rs, imm
         {"beqz", {2, {1}}},
         {"bnez", {2, {1}}},
         {"blez", {2, {1}}},
         {"bgez", {2, {1}}},
         {"bltz", {2, {1}}},
         {"bgtz", {2, {1}}},

         // Branch pseudos: rs1, rs2, imm
         {"bgt", {3, {2}}},
         {"ble", {3, {2}}},
         {"bgtu", {3, {2}}},
         {"bleu", {3, {2}}},

         // Jump pseudos
         {"j", {1, {0}}},
         {"jr", {1, {}}},
         {"call", {1, {0}}},
         {"tail", {1, {0}}},

         // M extension
         {"mul", {3, {}}},
         {"mulh", {3, {}}},
         {"mulhsu", {3, {}}},
         {"mulhu", {3, {}}},
         {"div", {3, {}}},
         {"divu", {3, {}}},
         {"rem", {3, {}}},
         {"remu", {3, {}}},
         {"mulw", {3, {}}},
         {"divw", {3, {}}},
         {"divuw", {3, {}}},
         {"remw", {3, {}}},
         {"remuw", {3, {}}},

         // F extension: rd, rs1, rs2
         {"flw", {3, {2}}},
         {"fsw", {3, {2}}},
         {"fadd.s", {3, {}}},
         {"fsub.s", {3, {}}},
         {"fmul.s", {3, {}}},
         {"fdiv.s", {3, {}}},
         {"fsqrt.s", {2, {}}},
         {"fsgnj.s", {3, {}}},
         {"fsgnjn.s", {3, {}}},
         {"fsgnjx.s", {3, {}}},
         {"fmin.s", {3, {}}},
         {"fmax.s", {3, {}}},
         {"fmadd.s", {4, {}}},
         {"fmsub.s", {4, {}}},
         {"fnmsub.s", {4, {}}},
         {"fnmadd.s", {4, {}}},
         {"fcvt.w.s", {2, {}}},
         {"fcvt.wu.s", {2, {}}},
         {"fmv.x.w", {2, {}}},
         {"feq.s", {3, {}}},
         {"flt.s", {3, {}}},
         {"fle.s", {3, {}}},
         {"fclass.s", {2, {}}},
         {"fcvt.s.w", {2, {}}},
         {"fcvt.s.wu", {2, {}}},
         {"fmv.w.x", {2, {}}},
         {"fcvt.l.s", {2, {}}},
         {"fcvt.lu.s", {2, {}}},
         {"fcvt.s.l", {2, {}}},
         {"fcvt.s.lu", {2, {}}},

         // D extension
         {"fld", {3, {2}}},
         {"fsd", {3, {2}}},
         {"fadd.d", {3, {}}},
         {"fsub.d", {3, {}}},
         {"fmul.d", {3, {}}},
         {"fdiv.d", {3, {}}},
         {"fsqrt.d", {2, {}}},
         {"fsgnj.d", {3, {}}},
         {"fsgnjn.d", {3, {}}},
         {"fsgnjx.d", {3, {}}},
         {"fmin.d", {3, {}}},
         {"fmax.d", {3, {}}},
         {"fmadd.d", {4, {}}},
         {"fmsub.d", {4, {}}},
         {"fnmsub.d", {4, {}}},
         {"fnmadd.d", {4, {}}},
         {"fcvt.s.d", {2, {}}},
         {"fcvt.d.s", {2, {}}},
         {"feq.d", {3, {}}},
         {"flt.d", {3, {}}},
         {"fle.d", {3, {}}},
         {"fclass.d", {2, {}}},
         {"fcvt.w.d", {2, {}}},
         {"fcvt.wu.d", {2, {}}},
         {"fcvt.d.w", {2, {}}},
         {"fcvt.d.wu", {2, {}}},
         {"fcvt.l.d", {2, {}}},
         {"fcvt.lu.d", {2, {}}},
         {"fmv.x.d", {2, {}}},
         {"fcvt.d.l", {2, {}}},
         {"fcvt.d.lu", {2, {}}},
         {"fmv.d.x", {2, {}}}};

    // const auto opcodeTable = getOpcodeTable();

    QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    // Accept commas in expansion lines (e.g., "sw r1, x2, 0") but reject
    // consecutive commas which would produce empty operands.
    QStringList normalizedParts;
    for (const QString &part : parts)
    {
        const QStringList subs = part.split(',', Qt::KeepEmptyParts);
        for (QString s : subs)
        {
            s = s.trimmed();
            if (s.isEmpty())
            {
                errorMessage = "Invalid expansion: empty operand (consecutive commas?)";
                return false;
            }
            normalizedParts << s;
        }
    }
    const QString opcode = normalizedParts.first().toLower();

    if (instruction_to_operand_count.count(opcode) == 0)
    {
        errorMessage = QString("Unknown opcode \"%1\" in expansion.").arg(opcode);
        return false;
    }

    const auto [expectedCount, immediatePositions] = instruction_to_operand_count.at(opcode);
    const int operandCount = normalizedParts.size() - 1;

    if (operandCount != expectedCount)
    {
        errorMessage = QString("Opcode \"%1\" expects %2 operands but got %3.")
                           .arg(opcode)
                           .arg(expectedCount)
                           .arg(operandCount);
        return false;
    }

    static const QRegularExpression validRegister(R"(^x([0-9]|[1-2][0-9]|3[0-1])$)");

    for (int i = 0; i < operandCount; i++)
    {
        const QString operand = normalizedParts[i + 1];

        if (immediatePositions.count(i) != 0)
        {
            bool ok;
            operand.toInt(&ok);
            if (!ok)
            {
                errorMessage = QString("Operand %1 of \"%2\" must be an integer, got \"%3\".")
                                   .arg(i + 1)
                                   .arg(opcode)
                                   .arg(operand);
                return false;
            }
        }
        else
        {
            const bool isDeclaredArg = declaredArgs.contains(operand);
            const bool isHardRegister = validRegister.match(operand).hasMatch();

            if (!isDeclaredArg && !isHardRegister)
            {
                errorMessage = QString("Operand \"%1\" in expansion of \"%2\" is not a "
                                       "declared arg or valid register.")
                                   .arg(operand)
                                   .arg(opcode);
                return false;
            }
        }
    }

    return true;
}

QJsonObject CustomPseudoManager::loadInstructionsFromDisk(QString &errorMessage)
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
    if (parseError.error != QJsonParseError::NoError)
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
