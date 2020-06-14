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

namespace detail{
	template <typename T1>
	constexpr auto get_first_element(T1 source)
	{
		static_assert(is_base_of_v<iterable, T1> || is_bounded_array_v<T1>);
		if constexpr(is_base_of_v<iterable, T1>){
			return *source.begin();
		}else{
			return *source;
		}
	}
	
	template <typename T1, typename T2>
	auto single_select(T1 source, T2 comparator)
	{
		auto result = get_first_element(source);
	
		for(const auto &i : source){
			result = comparator(i, result);
		}
		
		return result;
	}
};


template <typename T>
inline constexpr T min(T a, T b)
{
	return a > b ? b : a;
}

template <typename T>
inline constexpr T max(T a, T b)
{
	return a < b ? b : a;
}

template <typename T>
inline constexpr T clamp(T value, T a, T b)
{
	return min(max(value, a), b);
}

template <typename T1>
auto min(const T1 &source)
{
	static_assert(is_base_of_v<iterable, T1> || is_bounded_array_v<T1>);
	using element = decltype(detail::get_first_element(source));
	
	return detail::single_select(source, [](element a, element b){ return min(a, b); });
}

template <typename T1>
auto max(const T1 &source)
{
	static_assert(is_base_of_v<iterable, T1> || is_bounded_array_v<T1>);
	using element = decltype(detail::get_first_element(source));
	
	return detail::single_select(source, [](element a, element b){ return max(a, b); });
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


template <typename T1, typename T2>
T1 blacklist_copy(const T1 &source, const T2 &skips, int offset, int bound){
	static_assert(is_base_of_v<container, T1>);
	static_assert(is_base_of_v<iterable, T2>);
	T1 slice;
	for(const auto &i : skips){
		if(i < offset){
			continue;
		}else if(i < bound){
			slice.append(source.slice(offset, i));
			offset = i + 1;				
		}else{
			break;
		}
	}
	return slice.append(source.slice(offset, bound));
};

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

	int pivot = size / 2;
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

//offset only has a meaning for integer type evictees
//as there is no consistent value if say offset was applied to a hash array
template <typename T1, typename T2>
T1 &evict(T1 &target, int offset, const T2 &evictees)
{
	static_assert(is_base_of_v<container, T1>);
	static_assert(is_base_of_v<iterable, T2>);
	
	for(int count = 0; const auto &i : evictees){
		if constexpr(is_integral_v<decay<decltype(i)>::type>){
			target.remove(i - offset - count);
		}else{
			target.remove(i);
		}
		count++;
	}
}

template <typename T1, typename T2>
T1 &evict(T1 &target, const T2 &evictees)
{
	return evict(target, 0, evictees);
}
