#pragma once

#include <vector>

template<typename T, size_t Capacity>
class CircularBuffer
{
private:
    std::vector<T> m_Buffer;
    size_t m_Head = 0;
    size_t m_Tail = 0;

public:
    CircularBuffer()
    {
        m_Buffer.resize(Capacity);
    }

    void push(T value)
    {
        m_Buffer[m_Head] = value;
        m_Head = (m_Head + 1) % Capacity;
        if (m_Head == m_Tail)
        {
            m_Tail = (m_Tail + 1) % Capacity;
        }
    }

    class iterator
    {
    private:
        T* buf_;
        size_t tail_;
        size_t index_;

    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::random_access_iterator_tag;

        iterator() = default;
        iterator(CircularBuffer& buf, size_t idx) : buf_(buf.m_Buffer.data()), tail_(buf.m_Tail), index_(idx% Capacity) {}
        iterator(T* buf, size_t tail, size_t idx) : buf_(buf), tail_(tail), index_(idx% Capacity) {}

        T& operator*() const
        {
            return buf_[index_];
        }

        T* operator->() const
        {
            return &buf_[index_];
        }

        T operator[](difference_type n) const
        {
            return buf_[(index_ + n) % Capacity];
        }

        iterator& operator++()
        {
            index_ = (index_ + 1) % Capacity;
            return *this;
        }

        iterator& operator--()
        {
            index_ = (index_ - 1 + Capacity) % Capacity;
            return *this;
        }

        iterator& operator++(int amt)
        {
            index_ = (index_ + amt) % Capacity;
            return *this;
        }

        iterator& operator--(int amt)
        {
            index_ = (index_ - amt + Capacity) % Capacity;
            return *this;
        }

        iterator& operator+=(difference_type amt)
        {
            index_ = (index_ + amt) % Capacity;
            return *this;
        }
        iterator& operator-=(difference_type amt)
        {
            index_ = (index_ - amt + Capacity) % Capacity;
            return *this;
        }

        iterator operator-(int amt) const
        {
            size_t index = (index_ - amt + Capacity) % Capacity;
            return iterator{ buf_, tail_, index };
        }

        iterator operator+(int amt) const
        {
            size_t index = (index_ + amt) % Capacity;
            return iterator{ buf_, tail_, index };
        }

        bool operator!=(const iterator& other) const
        {
            return index_ != other.index_;
        }

        bool operator==(const iterator& other) const
        {
            return index_ == other.index_;
        }

        friend iterator operator+(difference_type n, const iterator& it)
        {
            return it + n;
        }

        friend iterator operator-(difference_type n, const iterator& it)
        {
            return it - n;
        }

        difference_type operator-(const iterator& rhs) const
        {
            int realIdx0 = index_ >= tail_ ? index_ - tail_ : Capacity - tail_ + index_;
            int realIdx1 = rhs.index_ >= rhs.tail_ ? rhs.index_ - rhs.tail_ : Capacity - rhs.tail_ + rhs.index_;
            return realIdx0 - realIdx1;
        }

    };

    iterator begin()
    {
        return iterator(*this, m_Tail);
    }

    iterator end()
    {
        return iterator(*this, m_Head);
    }

    bool empty() const
    {
        return m_Head == m_Tail;
    }

    bool full() const
    {
        return m_Tail == (m_Head + 1) % Capacity;
    }

    int head() const
    {
        return m_Head;
    }

    const std::vector<T>& buffer() const
    {
        return m_Buffer;
    }
};
