#pragma once
#include <cassert>
#include <cstddef>
#include "traits.h"
#include "algorithms.h"
#include "maybe.h"

template <typename T>
class array final : sortable, container, iterable{
	public:
		array() = default;
		array(const array<T> &other)
		{
			append(other);
		}
		
		array(initializer_list<T> list)
		{
			resize(list.size());
			copy(list.begin(), list.size(), data_);
		}
		
		array(array<T> &&move)
		{
			data_ = move.data_;
			length_ = move.length_;
			size_ = move.size_;
			
			move.data_ = nullptr;
			move.length_ = 0;
			move.size_ = 0;
		}
		
		array<T> &operator=(const array<T> &other)
		{
			resize(other.length());
			copy(other.data_, other.length(), data_);
			return *this;
		}
		
		T *data()
		{
			return data_;
		}
		
		const T *data() const
		{
			return data_;
		}
		
		int length() const
		{
			return length_;
		}
		
		int size() const
		{
			return size_;
		}
		
		T &operator[](int i)
		{
			return data_[i];
		}
		
		const T &operator[](int i) const
		{
			return data_[i];
		}
		
		array &operator+=(const T &other)
		{
			return append(other);
		}
		
		array &operator+=(const array<T> &other)
		{
			return append(other);
		}

		bool operator!=(const array<T> &other) const
		{
			return !(*this == other);
		}
		
		bool operator==(const array<T> &other) const
		{
			if(length_ != other.length_){
				return false;
			}
			
			for (int i = 0; i < length_; i++){
				if(data_[i] != other[i]){
					return false;
				}
			}
			
			return true;
		}
		
		void resize(int new_length)
		{
			if(new_length > size_){
				T *old_data = data_;
				size_ = bit_round(new_length + 1);
				data_ = move(old_data, min(length_, new_length), new T[size_]);
				delete []old_data;
			}
			length_ = new_length;
		}
		
		array slice(int start, int end) const
		{
			array<T> subarray;
			subarray.resize(end-start);
			copy(data_ + start, end - start, subarray.data_);
			return subarray;
		}
		
		array &append(const array<T> &other)
		{
			int current_end = length_;
			resize(length_ + other.length_);
			copy(other.data_, other.length_, data_ + current_end);
			return *this;
		}
		
		array &append(const T &item)
		{
			resize(length_ + 1);
			data_[length_ - 1] = item;
			return *this;
		}
		
		array &insert(const T &item, int index)
		{
			resize(length_ + 1);
			move(data_ + index, length_ - index, data_ + index + 1);
			data_[index] = item;
			return *this;
		}
		
		array &insert(const array<T> &other, int index)
		{
			int current_end = length_;
			resize(length_ + other.length_);
			move(data_ + index, length_ - index, data_ + index + other.length_);
			copy(other.data_, other.length_, data_ + index);
			return *this;
		}
		
		array &remove(int index)
		{
			move(data_ + index + 1, length_ - index - 1, data_ + index);
			length_--;
			return *this;
		}
		
		bool contains(const T &other) const
		{
			return find(0, length_, other) != -1;
		}
		
		maybe<int> find(int start, int end, const T &other) const
		{
			return fuzzy_find(start, end, [&other](auto a){
				return a == other;
			});
		}
		
		maybe<int> find(int start, const T &other) const
		{
			return find(start, length_, other);
		}
		
		maybe<int> find(const T &other) const
		{
			return find(0, length_, other);
		}
		
		template <typename F>
		maybe<int> fuzzy_find(int start, int end, const F &comparator) const
		{
			for(int i = start; i < end; i++){
				if constexpr(is_tuple_v<F>){
					int match_count = 0;
					for(int j = 0; j < comparator.size && j + i < end; j++){
						bool match = false;
						visit(comparator, j, [this, &match, i, j](auto call){ 
							match = call(data_[i+j]);
						});		
						match_count += match;
						if(!match){
							break;
						}				
					}
					if(match_count == comparator.size){
						return i;
					}
				}else{
					if(comparator(data_[i])){
						return i;
					}
				}
			}
			return nothing;
		}
		
		template <typename F>
		maybe<int> fuzzy_find(int start, const F &comparator) const
		{
			return fuzzy_find(start, length_, comparator);
		}
		
		template <typename F>
		maybe<int> fuzzy_find(const F &comparator) const
		{
			return fuzzy_find(0, length_, comparator);
		}

		template <typename F>
		array<int> fuzzy_find_all(int start, int end, const F &comparator) const
		{
			array<int> matches;
			for(auto i = fuzzy_find(start, end, comparator); i; i = fuzzy_find(i + 1, end, comparator)){
				matches += i;
			}
			return matches;
		}
		
		template <typename F>
		array<int> fuzzy_find_all(int start, const F &comparator) const
		{
			return fuzzy_find_all(start, length_, comparator);
		}		
		
		template <typename F>
		array<int> fuzzy_find_all(const F &comparator) const
		{
			return fuzzy_find_all(0, length_, comparator);
		}
	
		T *begin()
		{
			return data_;
		}
		
		T *end()
		{
			return data_ + length_;
		}
		
		const T *begin() const
		{
			return data_;
		}
		
		const T *end() const
		{
			return data_ + length_;
		}
		
		~array()
		{
			delete []data_;
		}
	
	private:
		T *data_ = nullptr;
		int length_ = 0;
		int size_ = 0;
};

static inline void test_array()
{
	array<int> x = { 1, 2, 3 };
	assert(x[0] == 1);
	assert(x[1] == 2);
	assert(x[2] == 3);
	x.append(4);
	x.append(5);
	assert(x[3] == 4);
	assert(x[4] == 5);
	
	{
		int i = 1;
		for(const auto &entry : x){
			assert(entry == i);
			i++;
		}
		assert(i == 6);
	}
	
	x.insert(4, 2);
	assert(x[2] == 4);
	assert(x[5] == 5);
	x.insert(4, 2);
	assert(x[6] == 5);
	assert(x[1] == 2);
}
