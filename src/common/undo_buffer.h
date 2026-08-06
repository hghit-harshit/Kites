#pragma once
namespace Kites
{
template <typename T>
class UndoBuffer
{
public:
    UndoBuffer(size_t capacity)
    :m_oldest(0), m_size(0)
    {
        m_data.reserve(capacity);
    }

    void setCapacity(size_t capacity)
    {
        clear();
        m_data.reserve(capacity);
    }
    [[nodiscard]] size_t capacity() const
    {
        return m_data.size();
    }

    void clear()
    {
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
        assert(capacity() > 0);
        if(m_current + 1 < m_size)
        {
            m_size = m_current + 1;
        }

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
        assert(capacity() > 0);
        if(m_current + 1 < m_size)
        {
            m_size = m_current + 1;
        }

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

    size_t physicalIndex(size_t logicalIndex) const
    {
        return (m_oldest + logicalIndex) % m_data.size();
    }
    std::vector<T> m_data;
    size_t m_oldest;
    size_t m_size;
    size_t m_current;
};
}
