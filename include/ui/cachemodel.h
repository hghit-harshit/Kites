#pragma once
#include <QAbstractTableModel>
#include "vm/memory_controller.h"

namespace Kites
{
class CacheModel : public QAbstractTableModel
{
    Q_OBJECT
    public:
        explicit CacheModel(QObject* parent = nullptr,MemoryController* cache = nullptr);
        int rowCount(const QModelIndex &parent= QModelIndex()) const override;
        int columnCount(const QModelIndex &parent= QModelIndex()) const override;
        QVariant data(const QModelIndex &index,int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    private:
        DirectMapCache* m_cache; // for now we will just have a direct map cahce 
        // later we'll update it to a more generic cache model that can handle different types of caches (fully associative, set associative)

    public slots:
        void updateCacheData(uint64_t address);
};
} // namespce Kites
