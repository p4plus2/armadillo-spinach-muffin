#pragma once
#include <cstdlib>
#include <cctype>
#include "algorithms.h"

template <typename T>
void swap(T &a, T &b)
{
	T temp = move(a);
	a = move(b);
	b = move(temp);
}

template <typename T1, typename T2>
struct pair{
	T1 first;
	T2 second;
	pair() = default;
	pair(T1 a, T2 b) : first(a), second(b){}
};

//swaps two variables, upon going out of scope they are swapped back
template <typename T>
class scoped_swap{
	T &a;
	T &b;
	public:
		scoped_swap(T &original, T &replacement) : a(original), b(replacement)
		{
			swap(a, b);
		}
		
		~scoped_swap()
		{
			swap(a, b);
		}
};

//temporarily replaces a value, upon going out of scope the original value is restored
template <typename T>
class scoped_temp{
	T &a;
	T b;
	public:
		scoped_temp(T &original, const T &replacement) : a(original), b(replacement)
		{
			swap(a, b);
		}
		
		~scoped_temp()
		{
			swap(a, b);
		}
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
