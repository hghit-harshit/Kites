#include "ui/cachemodel.h"

namespace Kites
{
    CacheModel::CacheModel(QObject* parent, MemoryController* memoryController)
        : QAbstractTableModel(parent)
    {
        if (!memoryController) 
        {
            qWarning() << "CacheModel: memoryController is null!";
            m_cache = nullptr;
            return;
        }

        m_cache = memoryController->GetCache();
        if(m_cache)
        {
            connect(memoryController, &MemoryController::memoryUpdated, this, &CacheModel::updateCacheData);
        }
    }

    int CacheModel::rowCount(const QModelIndex &parent) const
    {
        return NUM_LINES;
    }

    int CacheModel::columnCount(const QModelIndex &parent) const
    {
        return 5; // Index, Valid, Dirty, Tag, Data
    }

    QVariant CacheModel::data(const QModelIndex &index, int role) const
    {
        if (!m_cache || !index.isValid() || index.row() >= NUM_LINES)
            return QVariant();

        const auto& line = m_cache->GetCacheLineByIndex(index.row());

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
                case 0: //Index
                    return QString::number(index.row(),2).toUpper();
                case 1: // Valid
                    return line.valid ? "Yes" : "No";
                case 2: // Dirty
                    return line.dirty ? "Yes" : "No";
                case 3 : // Tag
                    return QString::number(line.tag, 16).toUpper();
                case 4: // Data (showing first 8 bytes for simplicity)
                    {
                        QString dataStr;
                        for (size_t i = 0; i < std::max<size_t>(8, LINE_SIZE); ++i)
                        {
                            dataStr += QString("%1 ").arg(line.data[i], 2, 16, QChar('0')).toUpper();
                        }
                        return dataStr.trimmed();
                    }
                default:
                    return QVariant();
            }
        }

        return QVariant();                                     
    }

    QVariant CacheModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section)
            {
                case 0 : return "Index";
                case 1: return "Valid";
                case 2: return "Dirty";
                case 3: return "Tag";
                case 4: return "Data";
                default: return QVariant();
            }
        }
        return QVariant();
    }

    void CacheModel::updateCacheData(uint64_t address)
    {
        // For now, we will just emit dataChanged for the entire model.
        // In a real implementation, you might want to be more specific about which rows/columns changed.
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    }
}// namespace Kites