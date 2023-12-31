#ifndef _CHUNK_ARRAY_
#define _CHUNK_ARRAY_

#include <cassert>
#include <vector>
#include <stdexcept>
#include <memory>
#include <iostream>

//#include "mark_array.h"

namespace HEM {

class ArrayBase;

template<typename T, uint32_t CHUNK_SIZE>
class ChunkArray;

/**
 * @brief ChunkArray 基类
 */
class ArrayBase 
{
public:
  //using MarkArray = ChunkArrayBool<1024u>;
  using MarkArray = ChunkArray<uint8_t, 1024u>;

public:
  ArrayBase(std::string name = "null"): name_(name) {}

  std::string get_name() { return name_; }
  const std::string get_name() const { return name_; }

  void set_name(std::string name) { name_ = name; }

  virtual ~ArrayBase() = 0;
  virtual void resize(size_t size) = 0;
  virtual void clear() = 0;
  virtual void copy_self(std::shared_ptr<MarkArray> &, std::shared_ptr<ArrayBase> & );

private:
  std::string name_;
};

ArrayBase::~ArrayBase() {}

void ArrayBase::resize(size_t ){}

void ArrayBase::clear() {}

void ArrayBase::copy_self(std::shared_ptr<MarkArray> &, std::shared_ptr<ArrayBase> & ) {}

/**
 * @brief 分块存储的数组
 * @param T         : 必须要有复制构造函数和复制 operator =
 * @param ChunkSize : 每个块的元素个数
 * @param size_     : 当前数组的长度
 * @param chunks_   : 每个块的指针
 */
template <typename T, uint32_t CHUNK_SIZE = 1024u>
class ChunkArray : public ArrayBase 
{
public:
  const constexpr static uint32_t ChunkSize = CHUNK_SIZE;
  using Self = ChunkArray<T, ChunkSize>;
  using Base = ArrayBase;

public:
  /**
   * @brief 构造函数
   */
  ChunkArray(std::string name = "null"): Base(name), size_(0), chunks_(0)
  { 
    chunks_.reserve(1024); 
  }

  /**
   * @brief 构造函数带 resize
   */
  ChunkArray(int size, std::string name = "null"): Base(name), size_(0), chunks_(0)
  {
    resize(size);
  }

  /**
   * @brief 复制构造函数
   */
  ChunkArray(const Self & other)
  {
    this->copy(other);
  }

  // 析构函数，释放分配的内存
  ~ChunkArray() override 
  {
    for (T* chunk : chunks_) 
      delete[] chunk;
  }

  T& operator[](size_t index)
  {
    assert(index < size_ && "Index out of range");
    size_t chunkIndex = index / ChunkSize;
    size_t offset = index % ChunkSize;
    return chunks_[chunkIndex][offset];
  }

  // 获取元素
  const T& operator[](size_t index) const
  {
    assert(index < size_ && "Index out of range");
    size_t chunkIndex = index / ChunkSize;
    size_t offset = index % ChunkSize;
    return chunks_[chunkIndex][offset];
  }

  T & back()
  {
    size_t chunkIndex = size_ / ChunkSize;
    size_t offset = size_ % ChunkSize;
    return chunks_[chunkIndex][offset];
  }

  const T & back() const
  {
    size_t chunkIndex = size_ / ChunkSize;
    size_t offset = size_ % ChunkSize;
    return chunks_[chunkIndex][offset];
  }

  T & get(size_t index) { return this->operator [](index);}

  const T & get(size_t index) const { return this->operator [](index);}

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
  void swap(Self & other) 
  {
    std::swap(chunks_, other.chunks_);
    std::swap(size_, other.size_);
  }

  void clear() override { size_ = 0;}

  // 复制另一个 ChunkArray 的内容
  void copy(const Self & other) 
  {
    if(this != &other)
    {
      this->set_name(other.get_name());
      resize(other.size_);
      size_t N_chunk = other.chunks_.size();
      for (size_t i = 0; i < N_chunk; ++i) 
        std::copy(other.chunks_[i], other.chunks_[i]+ChunkSize, chunks_[i]);
    }
  }

  /** @brief operator =  */
  Self & operator = (const Self & other)
  {
    this->copy(other);
    return *this;
  }

  /** @breif 预留空间，使得至少可以容纳指定数量的元素 */
  void reserve(size_t newCapacity) 
  {
    uint32_t cap = capacity();
    if (newCapacity > cap) 
    {
      uint32_t requiredChunks = (newCapacity + ChunkSize - 1) / ChunkSize;
      chunks_.resize(requiredChunks, nullptr);
      for (size_t i = cap/ChunkSize; i < requiredChunks; ++i) 
        chunks_[i] = new T[ChunkSize]();
    }
  }

  void resize(size_t newSize) override 
  {
    if (newSize <= capacity()) 
      size_ = newSize;  // 如果新大小小于等于当前大小，直接更新 size_
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
  class Iterator 
  {
  public:
    Iterator(Self & array, size_t index): array_(array), index_(index) {}

    bool operator!=(const Iterator& other) const { return index_ != other.index_; }

    Iterator & operator++() 
    {
      index_++;
      return *this;
    }

    T & operator*() { return array_[index_];}

    T * operator->() { return &array_[index_];}

  private:
    Self & array_;
    size_t index_;
  };

  // 迭代子的起始位置
  Iterator begin() { return Iterator(*this, 0); }

  // 迭代子的结束位置
  Iterator end() { return Iterator(*this, size_);}

private:
  size_t size_;  // 元素个数
  std::vector<T*> chunks_;  // 存储块的指针
};

/**
 * @brief 分块存储的数组
 * @param T         : 必须要有复制构造函数和复制 operator =
 * @param ChunkSize : 每个块的元素个数
 * @param size_     : 当前数组的长度
 * @param chunks_   : 每个块的指针
 */
template <typename T, size_t CHUNK_SIZE = 1024u>
class ChunkArrayWithMark : public ChunkArray<T, CHUNK_SIZE> 
{
public:
  using Base = ChunkArray<T, CHUNK_SIZE>;
  using Self = ChunkArrayWithMark<T, CHUNK_SIZE>;
  using MarkArray = typename Base::MarkArray;

public:
  /**
   * @brief 空的构造函数
   */
  ChunkArrayWithMark(): Base() {}

  /**
   * @brief 构造函数
   */
  ChunkArrayWithMark(std::string name, std::shared_ptr<MarkArray> mark): 
    Base(mark->size(), name), mark_(mark) {}

  void set_mark(std::shared_ptr<MarkArray> & mark) { mark_ = mark; }

  void copy_self(std::shared_ptr<MarkArray> & mark, std::shared_ptr<ArrayBase> & re) override
  {
    std::shared_ptr<Self> cp = std::make_shared<Self>(Base::get_name(), mark);
    cp->copy(*this);
    re = std::dynamic_pointer_cast<ArrayBase>(cp);
  }

public:
  // 迭代子
  class Iterator {
  public:
    Iterator(ChunkArrayWithMark & array, size_t index)
        : array_(array), index_(index) {}

    /** 左 ++ */
    Iterator & operator++() 
    {
      index_++;
      while(index_ < array_.size() && (array_.mark_->get(index_)==1))
        index_++;
      return *this;
    }

    /** 右 ++ */
    Iterator operator++(int) 
    {
      Iterator temp = *this;
      ++(*this);
      return temp;
    }

    bool operator!=(const Iterator& other) const { return index_ != other.index_; }

    T & operator*() { return array_[index_];}

    T * operator->() { return &array_[index_];}

  private:
    ChunkArrayWithMark & array_;
    size_t index_;
  };

  // 迭代子的起始位置
  Iterator begin() { return Iterator(*this, 0); }

  // 迭代子的结束位置
  Iterator end() { return Iterator(*this, this->size());}

private:
  std::shared_ptr<MarkArray> mark_;
};

}

#endif /* _CHUNK_ARRAY_ */ 

