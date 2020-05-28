#pragma once
#include "maybe.h"

class string;

class string_view{
	public:
		string_view(const char *data);
		string_view(const char *data, int length);
		string_view(const string &source);
		
		string to_string() const;
		
		char peek() const;
		char read();
		bool read_if(char c);
		
		int peek_int();
		int read_int();
		
		void shrink_prefix(int amount);
		void shrink_suffix(int amount);
		
		string_view substr(int start, int end = -1) const;
		
		bool starts_with(const string_view &other) const;
		bool ends_with(const string_view &other) const;
		bool contains(const string &other) const;
		
		const char *begin() const;
		const char *end() const;
		
		int length() const;
	private:
		const char *data_;
		int length_;
		
		int parse_int(bool advance_data);
};
