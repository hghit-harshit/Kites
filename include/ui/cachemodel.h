#pragma once
#include <QAbstractTableModel>
#include "vm/cache/cache.h"

namespace Kites
{


class CacheModel : public QAbstractTableModel
{
    Q_OBJECT
    public:
        explicit CacheModel(QObject* parent = nullptr,Cache* cache = nullptr);
        int rowCount(const QModelIndex &parent= QModelIndex()) const override;
        int columnCount(const QModelIndex &parent= QModelIndex()) const override;
        QVariant data(const QModelIndex &index,int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        void AttachCache(Cache* cache);
    public slots:
        void updateCacheData(uint64_t address);
        void updateCacheConfig(CacheConfig newConfig);

    private:

        size_t RowToSetIndex(int row) const { return row / num_ways_; }
        size_t RowToWayIndex(int row) const { return row % num_ways_; }
        int NumberOfWordColumns()     const { return static_cast<int>(block_size_ ); } // number of 4-byte words in a cache line

        static constexpr int COL_INDEX = 0;
        static constexpr int COL_VALID = 1;
        static constexpr int COL_DIRTY = 2;
        static constexpr int COL_TAG = 3;
        static constexpr int COL_DATA_START = 4; // starting column index for data bytes

        Cache* m_cache; 
        size_t num_sets_ = 0;
        size_t num_ways_ = 0;
        size_t block_size_ = 0;
    
    
   
    
};
} // namespce Kites
