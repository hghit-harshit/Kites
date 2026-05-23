#include "ui/vm_state_table_model.h"

namespace Kites
{
VMStateTableModel::VMStateTableModel(QObject *parent, VMManager *vmManager)
    : QAbstractTableModel(parent), m_vmManager(vmManager)
{
    connect(m_vmManager, &VMManager::vmStageChangedSignal, this,
            &VMStateTableModel::vmStateChangedSlot);
}

int VMStateTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 7;
}

int VMStateTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2; // Key and Value
}

QVariant VMStateTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if (section == 0)
            return QString("Key");
        else if (section == 1)
            return QString("Value");
    }
    return QVariant{};
}
QVariant VMStateTableModel::data(const QModelIndex &index, int role) const
{
    // Define the keys in the order they should appear
    // cuz we dont want to display all the keys in vm state map
    static const QStringList keys = {
        "ProgramCounter",      "Cycles", "InstructionsRetired", "CPI", "IPC", "StallCycles",
        "BranchMispredictions"};
    enum class VMStateKey
    {
        ProgramCounter,
        Cycles,
        InstructionsRetired,
        CPI,
        IPC,
        StallCycles,
        BranchMispredictions
    };
  
    if (!m_vmManager)
        return QVariant{};

    if (role == Qt::DisplayRole)
    {
        
        if (index.column() == 0)
        {
            return keys[index.row()];
        }
        else if (index.column() == 1)
        {
            switch (static_cast<VMStateKey>(index.row()))
            {
                case VMStateKey::ProgramCounter:
                    return m_vmManager->getProgramCounter();
                case VMStateKey::Cycles:
                    return m_vmManager->getCycles();
                case VMStateKey::InstructionsRetired:
                    return m_vmManager->getInstructionsRetired();
                case VMStateKey::CPI:
                    return m_vmManager->getCPI();
                case VMStateKey::IPC:
                    return m_vmManager->getIPC();
                case VMStateKey::StallCycles:
                    return m_vmManager->getStallCycles();
                case VMStateKey::BranchMispredictions:
                    return m_vmManager->getBranchMispredictions();
            }
        }
    }
    return QVariant{};
}

void VMStateTableModel::vmStateChangedSlot(const QMap<QString, QVariant> &vmState)
{
    Q_UNUSED(vmState);
    beginResetModel();
    endResetModel();
}
} // namespace Kites
