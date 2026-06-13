#include "ui/instruction_type_count_model.h"

namespace Kites
{ 
InstructionTypeCountModel::InstructionTypeCountModel(
    QObject *parent, 
    const std::array<size_t, static_cast<size_t>(InstructionType::NUMBER_OF_TYPES)> 
    &instruction_type_counts)
    : QAbstractTableModel(parent), m_instruction_type_counts(instruction_type_counts)
{}

int InstructionTypeCountModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2; // One column for the count
}

int InstructionTypeCountModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(InstructionType::NUMBER_OF_TYPES);
}

QVariant InstructionTypeCountModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (orientation == Qt::Horizontal)
    {
        if (section == 0)
        {
            return QString("Instruction Type");
        }
        else if (section == 1)
        {
            return QString("Count");
        }
    }
    return QVariant();
}


QVariant InstructionTypeCountModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (index.column() == 0)
    {
        // Return the instruction type name
        switch (static_cast<InstructionType>(index.row()))
        {
            case InstructionType::R_TYPE:
                return QString("R-Type");
            case InstructionType::I_TYPE:
                return QString("I-Type");
            case InstructionType::S_TYPE:
                return QString("S-Type");
            case InstructionType::B_TYPE:
                return QString("B-Type");
            case InstructionType::U_TYPE:
                return QString("U-Type");
            case InstructionType::J_TYPE:
                return QString("J-Type");
            case InstructionType::F_TYPE:
                return QString("F-Type");
            default:
                return QVariant();
        }
    }
    else if (index.column() == 1)
    {
        // Return the count
        size_t count = m_instruction_type_counts[static_cast<size_t>(index.row())];
        return QString::number(count);
    }

    return QVariant();
}

}//namespace Kites