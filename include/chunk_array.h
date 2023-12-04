#ifndef _CHUNK_ARRAY_
#define _CHUNK_ARRAY_

#include <iostream>
#include <cassert>
#include <vector>
#include <stdexcept>

/**
 * @brief ChunkArray 基类
 */
class ArrayBase 
{
public:
  ArrayBase(std::string name): name_(name) {}

  std::string get_name()
  {
    return name_;
  }

  const std::string get_name() const
  {
    return name_;
  }
private:
  std::string name_;
};

/**
 * @brief bool 类型的 chunk array, 使用 uint32_t 表示 32 位的 bool 值. 
 */
template <size_t ChunkSize = 1024u>
class ChunkArrayBool : public ArrayBase 
{
public:
  // 构造函数
  ChunkArrayBool(std::string name) : ArrayBase(name), size_(0), chunks_(0) {}

  // 构造函数
  ChunkArrayBool(std::string name, int size) : ArrayBase(name), size_(0), chunks_(0) 
  {
    resize(size);
  }

  // 析构函数，释放分配的内存
  ~ChunkArrayBool() {
      for (uint32_t* chunk : chunks_)
          delete[] chunk;
  }

  void set_true()
  {
    for(auto & chunk : chunks_)
      std::fill(chunk, chunk+(ChunkSize/32), 4294967295);
  }

  void set_false()
  {
    for(auto & chunk : chunks_)
      std::fill(chunk, chunk+(ChunkSize/32), 0);
  }

  // 设置指定位置的 bool 值
  void set_true(size_t index) 
  {
    assert(index < size_ && "Index out of range");
    size_t chunkIndex = index / ChunkSize;  // 计算是第几个 chunk
    size_t offset = index % ChunkSize;  // 计算在这个 chunk 中的偏移位置
    size_t intIndex = offset / 32;
    offset %= 32;
    chunks_[chunkIndex][intIndex] |= (1u << offset);  // 设置为 1
  }

  // 设置指定位置的 bool 值
  void set_fasle(size_t index) 
  {
    assert(index < size_ && "Index out of range");
    size_t chunkIndex = index / ChunkSize;  // 计算是第几个 chunk
    size_t offset = index % ChunkSize;  // 计算在这个 chunk 中的偏移位置
    size_t intIndex = offset / 32;
    offset %= 32;
    chunks_[chunkIndex][intIndex] &= ~(1u << offset);  // 设置为 0
  }

  // 获取指定位置的 bool 值
  bool get(size_t index) const 
  {
    assert(index < size_ && "Index out of range");
    size_t chunkIndex = index / ChunkSize;  // 计算是第几个 chunk
    size_t offset = index % ChunkSize;  // 计算在这个 chunk 中的偏移位置
    size_t intIndex = offset / 32;
    offset %= 32;
    return (chunks_[chunkIndex][intIndex] & (1u << offset)) != 0;  // 检查指定位置是否为 1
  }

  void push_back() { push_back_true(); }

  void push_back_true()
  {
    if (size_ == capacity())
      chunks_.emplace_back(new  uint32_t[ChunkSize/32]);
    set_true(size_);
    size_++;
  }

  void push_back_false()
  {
    if (size_ == capacity())
      chunks_.emplace_back(new  uint32_t[ChunkSize/32]);
    set_false(size_);
    size_++;
  }

  // 交换两个 ChunkArray 的内容
  void swap(ChunkArrayBool& other) 
  {
    std::swap(chunks_, other.chunks_);
    std::swap(size_, other.size_);
  }

  // 获取元素
  bool operator[](size_t index) { return get(index); }

  // 获取当前元素个数
  size_t size() const { return size_;}

  // 获取当前分配的内存容量
  size_t capacity() const { return chunks_.size() * ChunkSize;}

  void clear() 
  {
    size_ = 0;
  }

  // 复制另一个 ChunkArray 的内容
  void copy(const ChunkArrayBool& other) 
  {
    clear();
    reserve(other.size_);
    int N_chunk = other.chunks_.size();
    for (size_t i = 0; i < N_chunk; ++i) 
      std::copy(other.chunks_[i], other.chunks_[i]+ChunkSize/32, chunks_[i]);
  }

  // 预留空间，使得至少可以容纳指定数量的元素
  void reserve(size_t newCapacity) 
  {
    if (newCapacity > capacity()) 
    {
      size_t requiredChunks = (newCapacity + ChunkSize - 1) / ChunkSize;
      chunks_.resize(requiredChunks, nullptr);
      for (size_t i = capacity()/ChunkSize; i < requiredChunks; ++i) 
        chunks_[i] = new uint32_t[ChunkSize/32]();
    }
  }

  void resize(size_t newSize) 
  {
    reserve(newSize);  // 如果新大小大于当前大小，调用 reserve 函数
    size_ = newSize;
  }

private:
    size_t size_;  // 元素个数
    std::vector<uint32_t*> chunks_;  // 存储块的指针
};


/**
 * @brief 分块存储的数组
 * @param ChunkSize : 每个块的元素个数
 * @param size_     : 当前数组的长度
 * @param chunks_   : 每个块的指针
 */
template <typename T, size_t ChunkSize = 1024u>
class ChunkArray : public ArrayBase 
{
public:
  /**
   * @brief 构造函数
   */
  ChunkArray(const ChunkArrayBool<1024u> & mark, std::string name): 
    ArrayBase(name), size_(0), chunks_(0), mark_(mark)
  { 
    chunks_.reserve(1024); 
  }

  /**
   * @brief 构造函数带 resize
   */
  ChunkArray(const ChunkArrayBool<1024u> & mark, std::string name, int size): 
    ArrayBase(name), size_(0), chunks_(0), mark_(mark)
  {
    resize(size);
  }

  // 析构函数，释放分配的内存
  ~ChunkArray() 
  {
    for (T* chunk : chunks_) 
      delete[] chunk;
  }

  // 获取元素
  T& operator[](size_t index) 
  {
    assert(index < size_ && "Index out of range");
    size_t chunkIndex = index / ChunkSize;
    size_t offset = index % ChunkSize;
    return chunks_[chunkIndex][offset];
  }

  // 添加元素
  void push_back(const T& value) 
  {
    if (size_ == chunks_.size() * ChunkSize) 
      chunks_.push_back(new T[ChunkSize]);

    size_t chunkIndex = size_ / ChunkSize;
    size_t offset = size_ % ChunkSize;

    chunks_[chunkIndex][offset] = value;
    size_++;
  }

  /**
   * @brief emplace_back 类似于 std::vector
   */
  template <typename... Args>
  void emplace_back(Args&&... args)
  {
    if (size_ == capacity())
      chunks_.push_back(new T[ChunkSize]);

    size_t chunkIndex = size_ / ChunkSize;
    size_t offset = size_ % ChunkSize;

    chunks_[chunkIndex][offset] = T(std::forward<Args>(args)...);
    size_++;
  }

  // 获取当前元素个数
  size_t size() const { return size_;}

  // 获取当前分配的内存容量
  size_t capacity() const { return chunks_.size() * ChunkSize;}

  // 交换两个 ChunkedVector 的内容
  void swap(ChunkArray& other) 
  {
    std::swap(chunks_, other.chunks_);
    std::swap(size_, other.size_);
  }

  void clear() 
  {
    size_ = 0;
  }

  // 复制另一个 ChunkArray 的内容
  void copy(const ChunkArray& other) 
  {
    clear();
    reserve(other.size_);
    int N_chunk = other.chunks_.size();
    for (size_t i = 0; i < N_chunk; ++i) 
      std::copy(other.chunks_[i], other.chunks_[i]+ChunkSize, chunks_[i]);
  }

  // 预留空间，使得至少可以容纳指定数量的元素
  void reserve(size_t newCapacity) 
  {
    if (newCapacity > capacity()) 
    {
      size_t requiredChunks = (newCapacity + ChunkSize - 1) / ChunkSize;
      chunks_.resize(requiredChunks, nullptr);
      for (size_t i = capacity()/ChunkSize; i < requiredChunks; ++i) 
        chunks_[i] = new T[ChunkSize]();
    }
  }

  void resize(size_t newSize) 
  {
    if (newSize <= capacity()) 
    {
      size_ = newSize;  // 如果新大小小于等于当前大小，直接更新 size_
    } 
    else 
    {
      reserve(newSize);  // 如果新大小大于当前大小，调用 reserve 函数
      size_ = newSize;
    }
  }

  void set_value(const T & value)
  {
    for(auto chunk : chunks_)
      std::fill(chunk, chunk+ChunkSize, value);
  }


public:
  // 迭代子
  class iterator {
  public:
    iterator(ChunkArray& array, size_t index)
        : array_(array), index_(index) {}

    bool operator!=(const iterator& other) const { return index_ != other.index_; }

    iterator& operator++() 
    {
      index_++;
      while(index_ < array_.size_ && array_.mark_[index_])
      {
        index_++;
      }
      return *this;
    }

    T& operator*() { return array_[index_];}

  private:
    ChunkArray& array_;
    size_t index_;
  };

  // 迭代子的起始位置
  iterator begin() { return iterator(*this, 0); }

  // 迭代子的结束位置
  iterator end() { return iterator(*this, size_);}

private:
    size_t size_;  // 元素个数
    std::vector<T*> chunks_;  // 存储块的指针
    const ChunkArrayBool<1024u> & mark_;
};

#endif /* _CHUNK_ARRAY_ */ 
