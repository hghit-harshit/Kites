#include "cachemodel.h"
#include <QColor>
namespace Kites
{

CacheModel::CacheModel(QObject *parent, Cache *cache) : QAbstractTableModel(parent)
{
    if (!cache)
    {
        qWarning() << "CacheModel: cache is null!";
    }

    // m_cache = cache;
    attachCache(cache);
}

void CacheModel::attachCache(Cache *cache)
{
    beginResetModel();
    if (m_cache)
    {
        //disconnect everything from the old cache
        disconnect(m_cache,nullptr , this, nullptr);
    }
    m_cache = cache;
    if (m_cache)
    {
        connect(m_cache, &Cache::cacheLineUpdatedSignal, this, &CacheModel::updateCacheData);
        connect(m_cache, &Cache::cacheReconfiguredSignal, this,
                [this]()
                {
                    beginResetModel();
                    endResetModel();
                });
        connect(m_cache, &Cache::cacheMissSignal, this, &CacheModel::onCacheMiss);
        connect(m_cache, &Cache::cacheHitSignal, this, &CacheModel::onCacheHit);
    }

    endResetModel();
}

int CacheModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_cache->getSetCount() * m_cache->getWayCount());
}

int CacheModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(Column::DataStart) + 
    static_cast<int>(m_cache->getLineSizeInBytes() / sizeof(uint32_t));
}

QVariant CacheModel::data(const QModelIndex &index, int role) const
{
    if (!m_cache || !index.isValid() || index.row() >= rowCount() ||
        index.column() >= columnCount())
        return QVariant();

    size_t set_index = rowToSetIndex(index.row());
    size_t way_index = rowToWayIndex(index.row());
    if(role == Qt::UserRole + 1)
    {
        if (way_index == m_cache->getWayCount() - 1)
        {
            return 1; // last way in the set, draw a solid line
        }
        else 
        {
            return 0; // not the first way, draw a dotted line
        }
    }
    const CacheLine &line = m_cache->getCacheLine(set_index, way_index);

    if (role == Qt::BackgroundRole)
    {
        if (index.row() == m_last_hit_row)
        {
            return QColor(120, 220, 120, 90); // green for cache hit
        }

        if (index.row() == m_last_miss_row)
        {
            return QColor(255, 180, 180, 90); // red for cache miss
        }

        if (!line.valid)
        {
            return QVariant(); // default background
        }
        else if (line.dirty)
        {
            return QColor(255, 200, 100, 80); // light red for dirty lines
        }
    }

    if (role == Qt::TextAlignmentRole)
    {
        return Qt::AlignCenter;
    }

    if (role == Qt::DisplayRole)
    {
        Column column = static_cast<Column>(index.column());
        switch (column)
        {
        case Column::Index:
            return QString("%1").arg(set_index);
        case Column::Valid:
            return line.valid ? "1" : "0";
        case Column::Dirty:
            return line.dirty ? "1" : "0";
        case Column::Tag:
            if (!line.valid)
                return "0";
            else
                return QString("0x%1").arg(line.tag, 0, 16).toUpper();
        default: // data columns
        {
            if (!line.valid)
            {
                return "0";
            }

            int word_index = index.column() - static_cast<int>(Column::DataStart);
            size_t byte_offset = static_cast<size_t>(word_index) * 4; // 4 bytes per word

            if (byte_offset < m_cache->getLineSizeInBytes())
            {
                uint32_t word_data = 0;
                for (int i = 0; i < 4; ++i)
                {
                    if (byte_offset + i < m_cache->getLineSizeInBytes())
                    {
                        word_data |= static_cast<uint32_t>(line.data[byte_offset + i]) << (8 * i);
                    }
                }
                return QString("0x%1").arg(word_data, 0, 16).toUpper();
            }
            else
            {
                return QVariant(); // out of block size range
            }
        }
        }
    }

    if (role == Qt::ToolTipRole && index.column() >= static_cast<int>(Column::DataStart))
    {
        QString tooltip = QString("Set: %1, Way: %2\n").arg(set_index).arg(way_index);
        tooltip += line.valid ? "Valid\n" : "Invalid\n";
        tooltip += line.dirty ? "Dirty\n" : "Clean\n";
        if (line.valid)
        {
            tooltip += QString("Tag: 0x%1\n").arg(line.tag, 0, 16).toUpper();
            tooltip += "Data: ";
            for (size_t i = 0; i < m_cache->getLineSizeInBytes(); ++i)
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
        Column column = static_cast<Column>(section);
        switch (column)
        {
        case Column::Index:
            return "Index";
        case Column::Valid:
            return "Valid";
        case Column::Dirty:
            return "Dirty";
        case Column::Tag:
            return "Tag";
        default:
            if (section >= static_cast<int>(Column::DataStart))
            {
                int word_index = section - static_cast<int>(Column::DataStart);
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

int CacheModel::addressToRow(uint64_t address) const
{

    // We will highlight the first way of the set on a miss for simplicity
    return static_cast<int>(m_cache->getSetIndex(address) * m_cache->getWayCount());
}

int CacheModel::addressToHitRow(uint64_t address) const
{
    size_t setIndex = m_cache->getSetIndex(address);
    uint64_t tag = m_cache->getTag(address);
    for (size_t wayIndex = 0; wayIndex < m_cache->getWayCount(); ++wayIndex)
    {
        const CacheLine &line = m_cache->getCacheLine(setIndex, wayIndex);
        if (line.valid && line.tag == tag)
        {
            return static_cast<int>(setIndex * m_cache->getWayCount() + wayIndex);
        }
    }

    return static_cast<int>(setIndex * m_cache->getWayCount());
}

void CacheModel::updateCacheData(uint64_t address)
{
    Q_UNUSED(address);
    if (rowCount() == 0 || columnCount() == 0)
    {
        return;
    }
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

void CacheModel::updateCacheConfig(CacheConfig newConfig)
{
    beginResetModel();
    endResetModel();
}

void CacheModel::onCacheMiss(uint64_t address)
{
    if (!m_cache)
        return;

    m_last_miss_row = addressToRow(address);
    beginResetModel();
    endResetModel();
}

void CacheModel::onCacheHit(uint64_t address)
{
    if (!m_cache)
        return;

    m_last_hit_row = addressToHitRow(address);
    beginResetModel();
    endResetModel();
}

size_t CacheModel::rowToSetIndex(int row) const
{
    return row / m_cache->getWayCount();
}
size_t CacheModel::rowToWayIndex(int row) const
{
    return row % m_cache->getWayCount();
}
} // namespace Kites
