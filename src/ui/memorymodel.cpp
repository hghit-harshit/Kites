#include "ui/memorymodel.h"
#include "config.h"
#include <QDebug>
namespace Kites
{
MemoryModel::MemoryModel(QObject* parent, MemoryController* memoryController)
:QAbstractTableModel(parent)
{
    m_memoryController = memoryController;
    connect(m_memoryController,&MemoryController::memoryUpdated,this,&MemoryModel::updateMemory);
    connect(m_memoryController,&MemoryController::memoryResetSignal,this,&MemoryModel::memoryResetSlot);
}

int MemoryModel::rowCount(const QModelIndex &parent) const
{
    return m_rowsVisible;
}

int MemoryModel::columnCount(const QModelIndex &parent) const
{
    return 10; //address + 8 bytes + full double word
}

void MemoryModel::setRowsVisible(int rows)
{
    beginResetModel();
    m_rowsVisible = rows;
    endResetModel();
}

void MemoryModel::changeMemoryController(MemoryController* memoryController)
{
    beginResetModel();
    m_memoryController = memoryController;
    // the previous connection will be invalid now
    // and since previous vm as destroyed the connection auto disconnects
    connect(m_memoryController,&MemoryController::memoryUpdated,this,&MemoryModel::updateMemory);
    connect(m_memoryController,&MemoryController::memoryResetSignal,this,&MemoryModel::memoryResetSlot);
    endResetModel();
}
// bool MemoryModel::isValidAddress(const uint64_t& address, int offset) const
// {
//     if(offset < 0)
//     {
//         return (vm_config::config.getMemorySize() + offset*8 < )
//     }
// }

bool MemoryModel::canOffset(int offset)
{
    return ((offset < 0 && m_currentCentralAddress != 0) ||
    (offset > 0 && m_currentCentralAddress != vm_config::config.getMemorySize()));
}

void MemoryModel::offsetCentralAddress(int offset)
{
    if(!canOffset(offset))return;
    beginResetModel();
    m_currentCentralAddress =  m_currentCentralAddress + offset*8;
    // m_currentCentralAddress = (isValidAddress(newCentralAddress)
    // ? newCentralAddress : m_currentCentralAddress);
    // change it to get the bytes of rom f/lrom some kind of config
    endResetModel();
}

void MemoryModel::setCentralAddress(const uint64_t& address)
{
    beginResetModel();
    m_currentCentralAddress = (address/8)*8; // doing this becuase we want closed multiples of 8
    endResetModel();
}

QVariant MemoryModel::headerData(int section, Qt::Orientation orientation,int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant{};
    
    if(orientation == Qt::Horizontal)
    {
        switch(section)
        {
            case 0:
                return QString("Address");
            case 1:
                return QString("Double Word");
            case 2:
                return QString("Byte 0");
            case 3:
                return QString("Byte 1");
            case 4:
                return QString("Byte 2");
            case 5:
                return QString("Byte 3");
            case 6:
                return QString("Byte 4");
            case 7:
                return QString("Byte 5");
            case 8:
                return QString("Byte 6");
            case 9:
                return QString("Byte 7");
            default:
                return QVariant{};
        }
    }
}

QVariant MemoryModel::data(const QModelIndex &index,int role) const
{
    if(!m_memoryController)
    {
        if (role == Qt::DisplayRole)
            return QString("yo");
        else
            return QVariant();
    }
    
    if (role == Qt::TextAlignmentRole) 
    {
        return Qt::AlignCenter;
    }

    if(role == Qt::DisplayRole || role == Qt::ToolTipRole)
    {
        //if(!m_memoryController) return QString("hello");
        int offsetAddress = ((((m_rowsVisible * 8) / 2) / 8) * 8) - (index.row() * 8);
        //protecting against overflows
        if((offsetAddress  < 0 && abs(offsetAddress) > m_currentCentralAddress)
        || (offsetAddress > 0 && static_cast<uint64_t>(offsetAddress) 
        + m_currentCentralAddress > 
        vm_config::config.getMemorySize()))
        { return QString("-"); }
        const uint64_t alignedAddress =
      static_cast<uint64_t>(m_currentCentralAddress) + offsetAddress;
      

        //size_t row = static_cast<size_t>(index.row());
        // if(!isValidAddress(alignedAddress))
        // {
        //     return QString("-");
        // }
        //TODO 
        // refactor this swithch to dynamically give the byte based on column number
        // so that we have have an arbitrary byte sized block
        // for supporting 32 bit arch in future
        switch(index.column())
        {
            case 0: //Address
            {
                return QString("0x%1").arg(QString::number(alignedAddress,16).toUpper());
            }
            case 1: //Double Word
            {
                uint64_t doubleWord = m_memoryController->ReadDoubleWord(alignedAddress);
                return QString("0x%1").arg(QString::number(doubleWord,static_cast<int>(m_displayBase)).toUpper());
            }
            case 2: //Byte 0
            {
                uint8_t byte0 = m_memoryController->ReadByte(alignedAddress);
                return QString("0x%1").arg(QString::number(byte0,static_cast<int>(m_displayBase)).toUpper());
            }
            case 3: //Byte 1
            {
                uint8_t byte1 = m_memoryController->ReadByte(alignedAddress + 1);
                return QString("0x%1").arg(QString::number(byte1,static_cast<int>(m_displayBase)).toUpper());
            }
            case 4: //Byte 1
            {
                uint8_t byte2 = m_memoryController->ReadByte(alignedAddress + 2);
                return QString("0x%1").arg(QString::number(byte2,static_cast<int>(m_displayBase)).toUpper());
            }
            case 5: //Byte 3
            {
                uint8_t byte3 = m_memoryController->ReadByte(alignedAddress + 3);
                return QString("0x%1").arg(QString::number(byte3,static_cast<int>(m_displayBase)).toUpper());
            }
            case 6: //Byte 4
            {
                uint8_t byte4 = m_memoryController->ReadByte(alignedAddress + 4);
                return QString("0x%1").arg(QString::number(byte4,static_cast<int>(m_displayBase)).toUpper());
            }
            case 7: //Byte 5
            {
                uint8_t byte5 = m_memoryController->ReadByte(alignedAddress + 5);
                return QString("0x%1").arg(QString::number(byte5,static_cast<int>(m_displayBase)).toUpper());
            }
            case 8: //Byte 6
            {
                uint8_t byte6 = m_memoryController->ReadByte(alignedAddress + 6);
                return QString("0x%1").arg(QString::number(byte6,static_cast<int>(m_displayBase)).toUpper());
            }
            case 9: //Byte 7
            {
                uint8_t byte7 = m_memoryController->ReadByte(alignedAddress + 7);
                return QString("0x%1").arg(QString::number(byte7,static_cast<int>(m_displayBase)).toUpper());
            }
            
        }
    }

    return QVariant();
}

// bool MemoryModel::canFetchMore(const QModelIndex& index) const
// {
    
// }

// void MemoryModel::fetchMore(const QModelIndex &index)
// {
    
// }

void MemoryModel::memoryResetSlot()
{
    beginResetModel();
    endResetModel();
}

void MemoryModel::updateMemory(uint64_t address)
{
    /* QModelIndex topleft = this->index(static_cast<int>(address/8),0);
    QModelIndex bottomright = this->index(static_cast<int>(address/8),9);
    qDebug() << "Rows count:" << rowCount();
    qDebug() << "updating memory at address:" << QString::number(address,16).toUpper();
    qDebug() << "row:" << (address/8) - 1
         << "cols:" << 0 << "-" << 9
         << "valid?" << index(address/8, 0).isValid();
    emit dataChanged(topleft,bottomright,{Qt::DisplayRole,Qt::ToolTipRole}); */


    int64_t topAddr = static_cast<int64_t>(m_currentCentralAddress)
                    + static_cast<int64_t>(m_rowsVisible / 2) * 8;
    int64_t bytesFromTop = topAddr - static_cast<int64_t>(address);

    // address not in the visible window? nothing to update
    if (bytesFromTop < 0) return;

    int row = static_cast<int>(bytesFromTop / 8);
    if (row < 0 || row >= m_rowsVisible) return;

    QModelIndex topLeft = index(row, 0);
    QModelIndex bottomRight = index(row, columnCount() - 1);

    emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole, Qt::ToolTipRole});
}
}//namespace Kitesa
