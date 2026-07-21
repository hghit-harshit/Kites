#include "instruction_type_count_model.h"
#include "utils/to_index.h"
namespace Kites
{ 
namespace instr = instruction_set;

InstructionTypeCountModel::InstructionTypeCountModel(
    QObject *parent, 
    const std::array<size_t, toIndex(instr::InstructionType::INSTRUCTION_TYPE_COUNT)> 
    &instruction_type_counts)
    : QAbstractTableModel(parent), m_instructionTypeCounts(instruction_type_counts)
{}

int InstructionTypeCountModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2; // One column for the count
}

int InstructionTypeCountModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(instr::InstructionType::INSTRUCTION_TYPE_COUNT);
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
    Column column = static_cast<Column>(index.column());
    if (column == Column::InstructionType)
    {
        std::string typeName = instr::instructionTypeNames[static_cast<size_t>(index.row())];
        return QString::fromStdString(typeName);
        
    }
    else if (column == Column::InstructionCount)
    {
        // Return the count
        size_t count = m_instructionTypeCounts[static_cast<size_t>(index.row())];
        return QString::number(count);
    }

    return QVariant();
}

} // namespace kites
