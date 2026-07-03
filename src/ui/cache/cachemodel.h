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

    void attachCache(Cache *cache);
public slots:
    void updateCacheData(uint64_t address);
    void updateCacheConfig(CacheConfig newConfig);
    void onCacheMiss(uint64_t address);
    void onCacheHit(uint64_t address);

private:
    size_t rowToSetIndex(int row) const;
    size_t rowToWayIndex(int row) const;
    int addressToRow(uint64_t address) const;
    int addressToHitRow(uint64_t address) const;

    enum class Column
    {
        Index = 0,
        Valid,
        Dirty,
        Tag,
        DataStart 
    };

    Cache *m_cache = nullptr;
    int m_last_miss_row = -1;
    // bool m_miss_highlight_pending = false;
    int m_last_hit_row = -1;
    // bool m_hit_highlight_pending = false;
};
} // namespace Kites
