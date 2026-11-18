#pragma once
#include <cassert>
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace Kites
{
/**
 * Circular undo/redo history holding at most `capacity` entries; pushing past that
 * overwrites the oldest. A capacity of `unbounded` (0) keeps every entry instead.
 *
 * When entries are per-step *changes*, keep an empty entry first (push one after every
 * clear()), since canUndo() needs a step before the current one:
 *   undo: apply the reverse of current(), then undo();
 *   redo: if (auto entry = redo()) apply *entry.
 */
template <typename T>
class UndoBuffer
{
public:
    static constexpr size_t unbounded = 0;

    UndoBuffer(size_t capacity)
    :m_oldest(0), m_size(0), m_current(0), m_unbounded(capacity == unbounded)
    {
        m_data.resize(capacity); // resize, not reserve: capacity() is m_data.size()
    }

    void setCapacity(size_t capacity)
    {
        clear();
        m_unbounded = capacity == unbounded;
        m_data.clear();
        m_data.resize(capacity);
    }
    /// @brief Entries that fit before the oldest is overwritten (current storage if unbounded).
    [[nodiscard]] size_t capacity() const
    {
        return m_data.size();
    }

    void clear()
    {
        if(m_unbounded)
        {
            m_data.clear(); // release the history, like clearing a stack would
        }
        m_oldest = 0;
        m_size = 0;
        m_current = 0;
    }

    bool canUndo()
    {
        return m_current > 0;
    }

    bool canRedo()
    {
        return m_current + 1 < m_size;
    }

    const T& current() const
    {
        assert(m_current < m_size);
        return m_data[physicalIndex(m_current)];
    }

    void push(const T& value)
    {
        if(m_current + 1 < m_size)
        {
            m_size = m_current + 1;
        }
        growIfUnbounded();

        if(m_size < capacity())
        {
            m_data[physicalIndex(m_size)] = value;
            ++m_size;
        }
        else
        {
            m_data[m_oldest] = value;
            m_oldest = (m_oldest + 1) % m_data.size();
        }
        m_current = m_size - 1;
    }

    void push(T&& value)
    {
        if(m_current + 1 < m_size)
        {
            m_size = m_current + 1;
        }
        growIfUnbounded();

        if(m_size < capacity())
        {
            m_data[physicalIndex(m_size)] = std::move(value);
            ++m_size;
        }
        else
        {
            m_data[m_oldest] = std::move(value);
            m_oldest = (m_oldest + 1) % m_data.size();
        }
        m_current = m_size - 1;
    }

    std::optional<std::reference_wrapper<const T>> undo()
    {
        if(canUndo())
        {
            --m_current;
            return current();
        }
        return std::nullopt;
    }
    std::optional<std::reference_wrapper<const T>> redo()
    {
        if(canRedo())
        {
            ++m_current;
            return current();
        }
        return std::nullopt;
    }

private:

    void growIfUnbounded()
    {
        // Unbounded buffers never wrap (m_oldest stays 0): make room at the end instead.
        if(m_unbounded && m_size == m_data.size())
        {
            m_data.emplace_back();
        }
        assert(capacity() > 0);
    }

    size_t physicalIndex(size_t logicalIndex) const
    {
        return (m_oldest + logicalIndex) % m_data.size();
    }
    std::vector<T> m_data;
    size_t m_oldest;
    size_t m_size;
    size_t m_current;
    bool m_unbounded;
};
}
