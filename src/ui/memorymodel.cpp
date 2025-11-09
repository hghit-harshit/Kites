#include "ui/memorymodel.h"
#include "config.h"
namespace Kites
{
MemoryModel::MemoryModel(QObject* parent, MemoryController* memoryController)
:QAbstractTableModel(parent)
{
    m_memoryController = memoryController;
    connect(m_memoryController,&MemoryController::memoryUpdated,this,&MemoryModel::updateMemory);
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
    // change it to get the bytes of rom from some kind of config
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
                return QString("0x%1").arg(QString::number(doubleWord,16).toUpper());
            }
            case 2: //Byte 0
            {
                uint8_t byte0 = m_memoryController->ReadByte(alignedAddress);
                return QString("0x%1").arg(QString::number(byte0,16).toUpper());
            }
            case 3: //Byte 0
            {
                uint8_t byte1 = m_memoryController->ReadByte(alignedAddress + 1);
                return QString("0x%1").arg(QString::number(byte1,16).toUpper());
            }
            case 4: //Byte 0
            {
                uint8_t byte2 = m_memoryController->ReadByte(alignedAddress + 2);
                return QString("0x%1").arg(QString::number(byte2,16).toUpper());
            }
            case 5: //Byte 0
            {
                uint8_t byte3 = m_memoryController->ReadByte(alignedAddress + 3);
                return QString("0x%1").arg(QString::number(byte3,16).toUpper());
            }
            case 6: //Byte 0
            {
                uint8_t byte4 = m_memoryController->ReadByte(alignedAddress + 4);
                return QString("0x%1").arg(QString::number(byte4,16).toUpper());
            }
            case 7: //Byte 0
            {
                uint8_t byte5 = m_memoryController->ReadByte(alignedAddress + 5);
                return QString("0x%1").arg(QString::number(byte5,16).toUpper());
            }
            case 8: //Byte 0
            {
                uint8_t byte6 = m_memoryController->ReadByte(alignedAddress + 6);
                return QString("0x%1").arg(QString::number(byte6,16).toUpper());
            }
            case 9: //Byte 0
            {
                uint8_t byte7 = m_memoryController->ReadByte(alignedAddress + 7);
                return QString("0x%1").arg(QString::number(byte7,16).toUpper());
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

void MemoryModel::updateMemory(uint64_t address)
{
    QModelIndex topleft = this->index(static_cast<int>(address/8),0);
    QModelIndex bottomright = this->index(static_cast<int>(address/8),9);
    emit dataChanged(topleft,bottomright);
}

}//namespace Kitesa
