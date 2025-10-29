#include "ui/memorymodel.h"

namespace Kites
{
    MemoryModel::MemoryModel(QObject* parent, MemoryController* memoryController)
    :QAbstractTableModel(parent)
    {
        m_memoryController = memoryController;
        //connect(m_memoryController,&MemoryController::memoryUpdated,)
    }
}//namespace Kites