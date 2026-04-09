#include "ui/cachemodel.h"
#include <QColor>
namespace Kites
{
    
CacheModel::CacheModel(QObject* parent, Cache* cache)
    : QAbstractTableModel(parent)
{
    if (!cache)
    {
        qWarning() << "CacheModel: cache is null!";
    }

    m_cache = cache;
    if(m_cache)
    {
        connect(m_cache, &Cache::CacheLineUpdatedSignal, this, &CacheModel::updateCacheData);
        connect(m_cache, &Cache::CacheReconfiguredSignal, this, [this](){
            beginResetModel();
            num_sets_ = m_cache->GetNumSets();
            num_ways_ = m_cache->GetNumWays();
            block_size_ = m_cache->GetBlockSize();
            endResetModel();
        });
    }
}

void CacheModel::AttachCache(Cache* cache)
{
    beginResetModel();
    if (m_cache)
    {
        disconnect(m_cache, &Cache::CacheLineUpdatedSignal, this, &CacheModel::updateCacheData);
    }
    m_cache = cache;
    if(m_cache)
    {
        connect(m_cache, &Cache::CacheLineUpdatedSignal, this, &CacheModel::updateCacheData);
        num_sets_ = m_cache->GetNumSets();
        num_ways_ = m_cache->GetNumWays();
        block_size_ = m_cache->GetBlockSize();
    }
    
    endResetModel();

}

int CacheModel::rowCount(const QModelIndex &parent) const
{
    return static_cast<int>(num_sets_ * num_ways_);
}

int CacheModel::columnCount(const QModelIndex &parent) const
{
    return 4 + NumberOfWordColumns(); // Index, Valid, Dirty, Tag + Data columns
}

QVariant CacheModel::data(const QModelIndex &index, int role) const
{
    if (!m_cache || !index.isValid() || index.row() >= rowCount() || index.column() >= columnCount())
        return QVariant();

    size_t set_index = RowToSetIndex(index.row());
    size_t way_index = RowToWayIndex(index.row());

    const CacheLine& line = m_cache->GetCacheLine(set_index, way_index);

    if(role == Qt::BackgroundRole)
    {
        if(!line.valid)
        {
            return QVariant(); // default background
        }
        else if(line.dirty)
        {
            return QColor(255, 200, 100,80); // light red for dirty lines
        }
        return QColor(200, 255, 200,80); // light green for valid but clean lines
    }

    if(role == Qt::TextAlignmentRole)
    {
        return Qt::AlignCenter;
    }

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
            case COL_INDEX :
                return QString("%1").arg(set_index);
            case COL_VALID :
                return line.valid ? "1" : "0";    
            case COL_DIRTY :
                return line.dirty ? "1" : "0";
            case COL_TAG :
                if (!line.valid)
                    return "0";
                else
                    return QString("0x%1").arg(line.tag, 0, 16).toUpper();
            default: // data columns
            {
                if(!line.valid)
                {
                    return "0";
                }

                int word_index = index.column() - COL_DATA_START;
                size_t byte_offset = static_cast<size_t>(word_index) * 4; // 4 bytes per word

                if (byte_offset < block_size_)
                {
                    uint32_t word_data = 0;
                    for (int i = 0; i < 4; ++i)
                    {
                        if (byte_offset + i < block_size_)
                        {
                            word_data |= static_cast<uint32_t>(line.data[byte_offset + i]) << (8 * i);
                        }
                    }
                    return QString("0x%1").arg(word_data, 8, 16).toUpper();
                }
                else
                {
                    return QVariant(); // out of block size range
                }
            }
        }
    }
    
    if(role == Qt::ToolTipRole && index.column() >= COL_DATA_START)
    {
        QString tooltip = QString("Set: %1, Way: %2\n").arg(set_index).arg(way_index);
        tooltip += line.valid ? "Valid\n" : "Invalid\n";
        tooltip += line.dirty ? "Dirty\n" : "Clean\n";
        if(line.valid)
        {
            tooltip += QString("Tag: 0x%1\n").arg(line.tag, 0, 16).toUpper();
            tooltip += "Data: ";
            for(size_t i = 0; i < block_size_; ++i)
            {
                tooltip += QString("%1 ").arg(line.data[i], 2, 16, QChar('0')).toUpper();
            }
        }
        return tooltip;
    }

    return QVariant();                                     
}

QVariant CacheModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case COL_INDEX: return "Index";
            case COL_VALID: return "Valid";
            case COL_DIRTY: return "Dirty";
            case COL_TAG: return "Tag";
            default:
                if (section >= COL_DATA_START)
                {
                    int word_index = section - COL_DATA_START;
                    return QString("Data%1").arg(word_index);
                }
                else
                {
                    return QVariant();
                }
        }
    }
    return QVariant();
}

void CacheModel::updateCacheConfig(CacheConfig newConfig)
{
    beginResetModel();
    num_sets_ = newConfig.num_lines;
    num_ways_ = newConfig.num_ways;
    block_size_ = newConfig.block_size;
    endResetModel();
}

void CacheModel::updateCacheData(uint64_t address)
{
    // For now, we will just emit dataChanged for the entire model.
    // In a real implementation, you might want to be more specific about which rows/columns changed.
    if (!m_cache ||
        m_cache->GetNumSets()   != num_sets_  ||
        m_cache->GetNumWays()   != num_ways_  ||
        m_cache->GetBlockSize() != block_size_)
    {
        AttachCache(m_cache);  
        return;
    }

    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}
}// namespace Kites