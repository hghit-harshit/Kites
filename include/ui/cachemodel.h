#pragma once
#include "vm/cache/cache.h"
#include <QAbstractTableModel>
#include <cstdint>


namespace Kites
{

class CacheModel : public QAbstractTableModel
{
    Q_OBJECT
  public:
    explicit CacheModel(QObject *parent = nullptr, Cache *cache = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void AttachCache(Cache *cache);
  public slots:
    void updateCacheData(uint64_t address);
    void updateCacheConfig(CacheConfig newConfig);
    void onCacheMiss(uint64_t address);
    void onCacheHit(uint64_t address);

  private:
    size_t RowToSetIndex(int row) const
    {
        return row / m_num_ways;
    }
    size_t RowToWayIndex(int row) const
    {
        return row % m_num_ways;
    }
    int NumberOfWordColumns() const
    {
        return static_cast<int>(m_block_size);
    } // number of 4-byte words in a cache line
    int AddressToRow(uint64_t address) const;
    int AddressToHitRow(uint64_t address) const;

    static constexpr int COL_INDEX = 0;
    static constexpr int COL_VALID = 1;
    static constexpr int COL_DIRTY = 2;
    static constexpr int COL_TAG = 3;
    static constexpr int COL_DATA_START = 4; // starting column index for data bytes

    Cache *m_cache = nullptr;
    size_t m_num_sets = 0;
    size_t m_num_ways = 0;
    size_t m_block_size = 0;
    int m_last_miss_row = -1;
    // bool m_miss_highlight_pending = false;
    int m_last_hit_row = -1;
    // bool m_hit_highlight_pending = false;
};
} // namespace Kites
