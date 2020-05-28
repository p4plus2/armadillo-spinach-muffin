#pragma once

#include "traits.h"
#include <new>

struct nothing_t{};
static nothing_t nothing;

template <class T>
class maybe{
	public:
		maybe(){}
		maybe(nothing_t){}
		
		maybe(const T &source)
		{
			assign_T(source);
		}
		
		maybe(T &&source)
		{
			assign_T(move(source));
		}
		
		maybe(const maybe& source)
		{
			assign(source);
		}
		
		maybe(maybe&& source)
		{
			assign(move(source));
		}
		
		maybe<T> &operator=(nothing_t)
		{
			reset();
			return *this; 
		}
		
		maybe<T> &operator=(const T& source)
		{ 
			return assign_T(source);
		}
		
		maybe<T> &operator=(T&& source)
		{
			return assign_T(move(source));
		}

		maybe<T> &operator=(const maybe& source)
		{
			return assign(source);
		}

		maybe<T> &operator=(maybe&& source)
		{
			return assign(move(source));
		}
		
		bool valid()
		{
			return valid_;
		}
		
		void reset()
		{
			if(valid_){
				value_.~T();
			}
			valid_ = false;
		}
		
		T &get()
		{
			assert(valid_);
			return value_;
		}
		
		T &get_or(T &alternate)		//we can return lvalues by reference safely
		{
			return valid_ ? value_ : alternate;
		}
		
		T get_or(T &&alternate)		//rvalue can't be returned by reference safely
		{
			return valid_ ? value_ : alternate;
		}
		
		~maybe()
		{
			reset();
		}
	
	private:
		union{
			T value_;
			char dummy_;
		};
		
		bool valid_ = false;
		
		template<typename X>
		maybe<T> &assign(X &&source)
		{
			if(this == &source){
				return *this;
			}
			reset();
			valid_ = true; 
			new(&value_) T(forward<T>(source)); 
			return *this;
		}
		
		template<typename X>
		maybe<T> &assign_T(X &&source)
		{
			reset(); 
			valid_ = true; 
			new(&value_) T(forward<T>(source)); 
			return *this;
		}
			
};

template<typename T>
class maybe<T&>{
	public:
		maybe() : value_(nullptr) {}
		maybe(nothing_t) : value_(nullptr) {}
		maybe(const T& source) : value_((T*)&source) {}
		maybe(const maybe& source) : value_(source.value_) {}
		
		maybe &operator=(nothing_t)
		{
			value_ = nullptr; 
			return *this;
		}
		
		maybe &operator=(const T& source)
		{
			value_ = (T*)&source; 
			return *this;
		}
		
		maybe &operator=(const maybe& source)
		{
			value_ = source.value_; 
			return *this;
		}
		
		void reset()
		{
			value_ = nullptr;
		
		}
		
		T &get()
		{
			return *value_;
		}
	private:
		T* value_;
};
