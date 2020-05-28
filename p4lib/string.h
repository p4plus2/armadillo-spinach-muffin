#pragma once

#include <cstdio>
#include "traits.h"

class string{
	public:
		string(){};
		string(const char *cstring);
		string(const char *cstring, int end);
		string(const string &other);
		string(string &&move);
		string &operator=(const string &other);
		
		int length() const;
		const char *raw() const;
		char operator[](int i) const;
		string &operator+=(const string &other);
		string operator+(const char *other);
		string operator+(const string &other);
		bool operator!=(const string &other) const;
		bool operator==(const string &other) const;
		string &append(const string &other);
		string &append(const char c);
		string &fill(const char c, int count);
		string &left_pad(const char c, int count);
		string &right_pad(const char c, int count);
		void clear();
		bool starts_with(const string &other) const;
		bool ends_with(const string &other) const;
		bool contains(const string &other) const;
		string substr(int start, int end = -1) const;
		string trim() const;
		const char *begin() const;
		const char *end() const;
		~string();
	
	private:
		static const int max_inline_length_ = sizeof(char *) + sizeof(int) * 2 - 2;
		union{
			struct{
				char data_inline_contents_[max_inline_length_ + 1];
				unsigned char data_inline_length_;
			};
			struct{
				char *data_ = nullptr;
				int length_ = 0;
				int size_ = 0;
			};
		};
		
		char *data();
		bool inlined() const;
		void resize(int new_length);
		void set_length(int length);
		void copy_string(const string &other);
		void copy_cstring(const char *cstring, int size);
};

inline string to_string(char c)
{
	return string().append(c);
}

template <typename T, typename = enable_if_t<is_integral_v<T>>>
string to_string(T value, int base = 10)
{
	static char buffer[sizeof value * 8 + 2];
	static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	if(base < 2 || base > 36){
		return string();
	}

	char* p = &buffer[sizeof(buffer) - 1];
	*p = '\0';

	T n = value;
	if constexpr(is_signed_v<T>){
		n = value < 0 ? value : -value; 
	} 

	do{
		if constexpr(is_signed_v<T>){
			*(--p) = digits[-(n % base)];
		}else{
			*(--p) = digits[(n % base)];
		}
		n /= base;
	}while(n);
	
	if constexpr(is_signed_v<T>){
		if(value < 0){
			*(--p) = '-';
		}
	}

	return string(p);
}

void test_string();
