#include <cstring>
#include <cstdlib>
#include <cassert>
#include "string_view.h"
#include "string.h"

string_view::string_view(const char *data)
{
	data_ = data;
	length_ = strlen(data);
}
string_view::string_view(const char *data, int length)
{
	data_ = data;
	length_ = length;
}
string_view::string_view(const string &source)
{
	data_ = source.raw();
	length_ = source.length();
}

string string_view::to_string() const
{
	return string(data_, length_);
}

char string_view::peek() const
{
	return *data_;
}
char string_view::read()
{
	length_--;
	assert(length() >= 0);
	return *data_++;
}

bool string_view::read_if(char c)
{
	return (peek() == c) ? (read() == c) : false;
}

int string_view::peek_int()
{
	return parse_int(false);
}
int string_view::read_int()
{
	return parse_int(true);
}

void string_view::shrink_prefix(int amount)
{
	data_ += amount;
	length_ -= amount;
}
void string_view::shrink_suffix(int amount)
{
	length_ -= amount;
}

string_view string_view::substr(int start, int end) const
{
	assert(end <= length());
	return string_view(data_ + start, ((end == -1) ? length_ : end) - start);
}

bool string_view::starts_with(const string_view &other) const
{
	if(other.length() > length()){
		return false;
	}
	return !memcmp(data_, other.data_, other.length_);
}
bool string_view::ends_with(const string_view &other) const
{
	if(other.length() > length()){
		return false;
	}
	return !memcmp(data_ + length_ - other.length_, other.data_, other.length_);
}


bool string_view::contains(const string &other) const
{
	if(other.length() > length()){
		return false;
	}
	return strstr(data_, other.raw());
}

const char *string_view::begin() const
{
	return data_;
}
const char *string_view::end() const
{
	return data_ + length_;
}

int string_view::length() const
{
	return length_;
}

int string_view::parse_int(bool advance_data)
{
	char *end;
	int value = strtol(data_, &end, 0);
	length_ -= advance_data ? (end - data_) : 0;
	data_ = advance_data ? end : data_;
	return value;
}
