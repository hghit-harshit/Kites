#include <cstddef>
#include <QMetaType>
#pragma once

namespace Kites
{
enum class WritePolicy
{
    WriteThrough = 0,
    WriteBack
};

enum class AllocationPolicy
{
    WriteAllocate = 0,
    NoWriteAllocate
};

enum class ReplacementPolicy : size_t
{
    LRU = 0,
    FIFO,
    Custom
};

struct CacheConfig
{
    size_t lineCount;
    size_t lineSizeInBytes;
    size_t wayCount;
    WritePolicy writePolicy;
    AllocationPolicy allocationPolicy;
    ReplacementPolicy replacementPolicy;
};

struct CacheStats
{
    size_t hitCount         = 0;
    size_t missCount        = 0;
    size_t writeBackCount   = 0;
    double hitRate          = 0.0;
    size_t cacheSizeInBytes = 0;
};

}//namespace Kites

//for making the enums usable in Qt's combo box
Q_DECLARE_METATYPE(Kites::WritePolicy)
Q_DECLARE_METATYPE(Kites::AllocationPolicy)
Q_DECLARE_METATYPE(Kites::ReplacementPolicy)