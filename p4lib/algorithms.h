#pragma once

#include "traits.h"
#include "utility.h"
#include <cstring>

inline constexpr int bit_round(int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

inline constexpr int min(int a, int b)
{
	return a > b ? b : a;
}

inline constexpr int max(int a, int b)
{
	return a < b ? b : a;
}

inline constexpr int clamp(int value, int a, int b)
{
	return min(max(value, a), b);
}

template <typename T>
T *copy(const T *source, int copy_length, T *dest)
{
	if(source && dest){
		if constexpr(is_trivially_copyable_v<T>){
			memmove(dest, source, copy_length*sizeof(T));
		}else{
			if(source > dest){
				for (int i = 0; i < copy_length; i++){
					dest[i] = source[i];
				}
			}else{
				for (int i = copy_length-1; i >= 0; i--){
					dest[i] = source[i];
				}				
			}
		}
	}
	return dest;
}

template <typename T>
T *move(T *source, int copy_length, T *dest)
{
	if(source && dest){
		if constexpr(is_trivially_copyable_v<T>){
			memmove(dest, source, copy_length*sizeof(T));
		}else{
			if(source > dest){
				for (int i = 0; i < copy_length; i++){
					dest[i] = move(source[i]);
				}
			}else{
				for (int i = copy_length-1; i >= 0; i--){
					dest[i] = move(source[i]);
				}				
			}
		}
	}
	return dest;
}


//template<typename T> using has_to_string = decltype(declval<T&>().to_string());
//constexpr(is_detected_exact_v<string, has_to_string, T>)

template <typename T, typename C>
T *sort(T *data, int size, const C &comparator)
{
	if(size <= 1){
		return data;
	}
	
	if(size < 64){	//todo config mechanism (and test for good cut off)
		for(int i = 0, j; i < size; i++){
			auto current = move(data[i]);
			for(j = i - 1; j >= 0; j--){
				if(comparator(current, data[j])){
					data[j + 1] = move(data[j]);
				}else{
					break;
				}
			}
			data[j + 1] = move(current);
		}
	}

	uint pivot = size / 2;
	sort(data, pivot, comparator);
	sort(data + pivot, size - pivot, comparator);

	auto buffer = allocate<T>(size);
	int offset = 0;
	int left = 0;
	int right = pivot;
	
	while(left < pivot && right < size) {
		if(comparator(data[right], data[left])) {
			new(buffer + offset++) T(move(data[right++]));
		} else {
			new(buffer + offset++) T(move(data[left++]));
		}
	}
	while(left < pivot){
		new(buffer + offset++) T(move(data[left++]));
	}
	while(right < size ){
		new(buffer + offset++) T(move(data[right++]));
	}
	
	for(int i = 0; i < size; i++) {
		data[i] = move(buffer[i]);
		buffer[i].~T();
	}
	deallocate(buffer);
	return data;
}

template <typename T, typename C>
T *sort(T *data, int size)
{
	return sort(data, size, [](auto a, auto b){ return a < b; });
}

template <typename T, typename C>
T &sort(T &target, const C &comparator)
{
	static_assert(is_base_of_v<sortable, T>);
	sort(target.data(), target.length(), comparator);
	return target;
}

template <typename T>
T &sort(T &target)
{
	return sort(target, [](auto a, auto b){ return a < b; });
}
