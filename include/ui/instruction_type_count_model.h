#pragma once
#include <array>
#include <QAbstractTableModel>
#include "common/instruction_types.h"

namespace Kites
{
class InstructionTypeCountModel : public QAbstractTableModel
{
    public:
    InstructionTypeCountModel(QObject *parent = nullptr,
        const std::array<size_t, static_cast<size_t>(InstructionType::NUMBER_OF_TYPES)>
                                    &instruction_type_counts = {});
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    
    private:
    // using const here because the model should not modify the counts, 
    // it should only display them
    const std::array<size_t, static_cast<size_t>(InstructionType::NUMBER_OF_TYPES)> 
    &m_instruction_type_counts;
};  
}//namespace Kites
