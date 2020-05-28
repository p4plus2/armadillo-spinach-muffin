#pragma once
#include <cstdlib>
#include <cctype>

template <typename T1, typename T2>
struct pair{
	T1 first;
	T2 second;
};

//these are useful in contexts where you want raw memory
//usually combined with placement new
template<typename T> T *allocate(int size)
{
	return (T*)malloc(size * sizeof(T));
}

inline void deallocate(void* target)
{
	free(target);
}


inline bool isidentstart(char c)
{
	return isalpha(c) || c == '_';
}

inline bool isident(char c)
{
	return isalnum(c) || c == '_';
}
