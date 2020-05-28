#pragma once

#include "traits.h"

template<typename T> using has_hash = decltype(declval<T&>().hash());
template<typename T> using has_hash_free = decltype(hash(declval<T&>()));

typedef hash_type uint64_t;	//typedef to allow easy future changes.

template<typename T>
hash_type get_hash(T const& t)
{
	if constexpr(is_detected_exact_v<hash_type, has_hash, T>){
		return t.hash();
	}else if constexpr(is_detected_exact_v<hash_type, has_hash_free, T>){
		return hash(t);
	}
	
	static_assert(false, "No valid hash method found");
}


hash_type hash_memory(const char *memory, int length)
{
	hash_type r = 0;
	
	for(int i = 0; i < length; i++){
		r = r * 67 + memory[i] - 113;  
	}
	
	return r;
}

hash_type hash(int i)
{
	return i;
}
