#pragma once
#include <array>
#include <QAbstractTableModel>
#include "common/instruction_types.h"

namespace Kites
{


class InstructionTypeCountModel : public QAbstractTableModel
{
    using InstructionTypeCounts = std::array<size_t, 
    static_cast<size_t>(instruction_set::InstructionType::INSTRUCTION_TYPE_COUNT)>;
    public:
    InstructionTypeCountModel(QObject *parent = nullptr,
        const InstructionTypeCounts &instruction_type_counts = {});
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    
    private:

    enum class Column
    {
        INSTRUCTION_TYPE = 0,
        COUNT = 1
    };
    // using const here because the model should not modify the counts, 
    // it should only display them
    const InstructionTypeCounts &m_instruction_type_counts;
};  
} // namespace kites

