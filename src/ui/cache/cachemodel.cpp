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
    AttachCache(cache);
}

void CacheModel::AttachCache(Cache *cache)
{
    beginResetModel();
    if (m_cache)
    {
        disconnect(m_cache, &Cache::cacheLineUpdatedSignal, this, &CacheModel::updateCacheData);
    }
    m_cache = cache;
    if (m_cache)
    {
        m_num_sets = m_cache->getSetCount();
        m_num_ways = m_cache->getWayCount();
        m_block_size = m_cache->getLineSizeInBytes();

        connect(m_cache, &Cache::cacheLineUpdatedSignal, this, &CacheModel::updateCacheData);
        connect(m_cache, &Cache::cacheReconfiguredSignal, this,
                [this]()
                {
                    beginResetModel();
                    m_num_sets = m_cache->getSetCount();
                    m_num_ways = m_cache->getWayCount();
                    m_block_size = m_cache->getLineSizeInBytes();
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
    return static_cast<int>(m_num_sets * m_num_ways);
}

int CacheModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4 + NumberOfWordColumns(); // Index, Valid, Dirty, Tag + Data columns
}

QVariant CacheModel::data(const QModelIndex &index, int role) const
{
    if (!m_cache || !index.isValid() || index.row() >= rowCount() ||
        index.column() >= columnCount())
        return QVariant();

    size_t set_index = RowToSetIndex(index.row());
    size_t way_index = RowToWayIndex(index.row());

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
        switch (index.column())
        {
        case COL_INDEX:
            return QString("%1").arg(set_index);
        case COL_VALID:
            return line.valid ? "1" : "0";
        case COL_DIRTY:
            return line.dirty ? "1" : "0";
        case COL_TAG:
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

            int word_index = index.column() - COL_DATA_START;
            size_t byte_offset = static_cast<size_t>(word_index) * 4; // 4 bytes per word

            if (byte_offset < m_block_size * 4)
            {
                uint32_t word_data = 0;
                for (int i = 0; i < 4; ++i)
                {
                    if (byte_offset + i < m_block_size * 4)
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

    if (role == Qt::ToolTipRole && index.column() >= COL_DATA_START)
    {
        QString tooltip = QString("Set: %1, Way: %2\n").arg(set_index).arg(way_index);
        tooltip += line.valid ? "Valid\n" : "Invalid\n";
        tooltip += line.dirty ? "Dirty\n" : "Clean\n";
        if (line.valid)
        {
            tooltip += QString("Tag: 0x%1\n").arg(line.tag, 0, 16).toUpper();
            tooltip += "Data: ";
            for (size_t i = 0; i < m_block_size * 4; ++i)
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
        case COL_INDEX:
            return "Index";
        case COL_VALID:
            return "Valid";
        case COL_DIRTY:
            return "Dirty";
        case COL_TAG:
            return "Tag";
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

int CacheModel::AddressToRow(uint64_t address) const
{
    if (m_block_size == 0 || m_num_ways == 0)
        return -1;

    size_t block_offset_bits =
        static_cast<size_t>(std::log2(m_block_size * 4)); // block size in bytes
    size_t set_index_bits = static_cast<size_t>(std::log2(m_num_sets));

    size_t set_index = (address >> block_offset_bits) & ((1 << set_index_bits) - 1);

    // We will highlight the first way of the set on a miss for simplicity
    return static_cast<int>(set_index * m_num_ways);
}

int CacheModel::AddressToHitRow(uint64_t address) const
{
    if (!m_cache || m_block_size == 0 || m_num_ways == 0 || m_num_sets == 0)
        return -1;

    size_t block_offset_bits = static_cast<size_t>(std::log2(m_block_size * 4));
    size_t set_index_bits = static_cast<size_t>(std::log2(m_num_sets));

    size_t set_index = (address >> block_offset_bits) & ((1ULL << set_index_bits) - 1);
    uint64_t tag = address >> (block_offset_bits + set_index_bits);

    for (size_t way_index = 0; way_index < m_num_ways; ++way_index)
    {
        const CacheLine &line = m_cache->getCacheLine(set_index, way_index);
        if (line.valid && line.tag == tag)
        {
            return static_cast<int>(set_index * m_num_ways + way_index);
        }
    }

    return static_cast<int>(set_index * m_num_ways);
}

void CacheModel::updateCacheData(uint64_t address)
{
    Q_UNUSED(address);
    if (!m_cache || m_cache->getSetCount() != m_num_sets || m_cache->getWayCount() != m_num_ways ||
        m_cache->getLineSizeInBytes() != m_block_size)
    {
        AttachCache(m_cache);
        return;
    }

    if (rowCount() == 0 || columnCount() == 0)
    {
        return;
    }

    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

void CacheModel::updateCacheConfig(CacheConfig newConfig)
{
    beginResetModel();
    m_num_sets = newConfig.lineCount;
    m_num_ways = newConfig.wayCount;
    m_block_size = newConfig.lineSizeInBytes;
    endResetModel();
}

void CacheModel::onCacheMiss(uint64_t address)
{
    if (!m_cache)
        return;

    m_last_miss_row = AddressToRow(address);
    beginResetModel();
    endResetModel();
}

void CacheModel::onCacheHit(uint64_t address)
{
    if (!m_cache)
        return;

    m_last_hit_row = AddressToHitRow(address);
    beginResetModel();
    endResetModel();
}
} // namespace Kites
