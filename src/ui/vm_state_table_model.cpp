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

    static const QList<std::function<QVariant()>> valueGetters = {
        [this]() { return m_vmManager->getProgramCounter(); },
        [this]() { return m_vmManager->getCycles(); },
        [this]() { return m_vmManager->getInstructionsRetired(); },
        [this]() { return m_vmManager->getCPI(); },
        [this]() { return m_vmManager->getIPC(); },
        [this]() { return m_vmManager->getStallCycles(); },
        [this]() { return m_vmManager->getBranchMispredictions(); }};

    if (!m_vmManager)
        return QVariant{};

    if (role == Qt::DisplayRole)
    {
        // Get the VM state map from the VMManager
        //  const QMap<QString, QVariant>& vmState = m_vmManager->getVMStateMap();
        //  if (index.row() < 0 || index.row() >= vmState.size())
        //      return QVariant{};

        // auto it = vmState.constBegin();
        // std::advance(it, index.row());

        if (index.column() == 0)
        {
            return keys[index.row()];
        }
        else if (index.column() == 1)
        {
            return valueGetters[index.row()]();
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
