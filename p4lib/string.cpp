#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cctype>
#include <utility>
#include "algorithms.h"
#include "string.h"
#include <cstdio>

string::string(const char *cstring)
{
	copy_cstring(cstring, strlen(cstring));
}

string::string(const char *cstring, int end)
{
	copy_cstring(cstring, end);
}

string::string(const string &other)
{
	copy_string(other);
}

string::string(string &&move)
{
	delete []data_;
	if(!move.inlined()){
		data_ = move.data_;
		length_ = move.length_;
		size_ = move.size_;
		move.data_ = nullptr;
		move.length_ = 0;
		move.size_ = 0;
	}else{
		copy_string(move);
	}
}

string &string::operator=(const string &other)
{
	copy_string(other);
	return *this;
}

int string::length() const
{
	return inlined() ? data_inline_length_ : length_;
}

const char *string::raw() const
{
	return inlined() ? data_inline_contents_ : data_;
}

char string::operator[](int i) const
{
	return raw()[i];
}

string &string::operator+=(const string &other)
{
	return append(other);
}

string string::operator+(const char *other)
{
	string temp(other);
	string temp2 = *this;
	return temp2.append(temp);
}


string string::operator+(const string &other)
{
	string temp = *this;
	return temp.append(other);
}


bool string::operator!=(const string &other) const
{
	return !(*this == other);
}

bool string::operator==(const string &other) const
{
	return other.length() == length() && !strcmp(other.raw(), raw());
}

string &string::append(const string &other)
{
	int current_end = length();
	resize(length() + other.length());
	copy(other.raw(), other.length(), data() + current_end);
	return *this;
}

string &string::append(const char c)
{
	resize(length() + 1);
	data()[length() - 1] = c;
	return *this;
}

string &string::fill(const char c, int count)
{
	resize(count);
	for(int i = 0; i < count; i++){
		
		data()[i] = c;
	}
	
	return *this;
}

string &string::left_pad(const char c, int count)
{
	if(length() > count){
		return *this;
	}
	string padding;
	padding.fill(c, count - length());
	padding.append(*this);
	*this = std::move(padding);
	return *this;
}

string &string::right_pad(const char c, int count)
{
	if(length() > count){
		return *this;
	}
	while(length() < count){
		append(c);
	}
	return *this;
}

bool string::starts_with(const string &other) const
{
	if(other.length() > length()){
		return false;
	}
	return !memcmp(raw(), other.raw(), other.length());
}

bool string::ends_with(const string &other) const
{
	if(other.length() > length()){
		return false;
	}
	return !memcmp(raw() + length() - other.length(), other.raw(), other.length());
}

bool string::contains(const string &other) const
{
	if(other.length() > length()){
		return false;
	}
	return strstr(raw(), other.raw());
}

string string::substr(int start, int end) const
{
	if(end == -1 || end - start > length()){
		return string(raw()+start);
	}
	return string(raw()+start, end-start);
}

string string::trim() const
{
	int start = 0;
	int end = length();
	while(end > start && isspace(raw()[end-1])){
		 end--;
	}
	while(start < end && isspace(raw()[start])){
		start++;
	}
	return substr(start, end);
}

void string::clear()
{
	resize(0);
}

const char *string::begin() const
{
	return raw();
}

const char *string::end() const
{
	return raw() + length();
}

string::~string()
{
	if(!inlined()){
		delete []data_;
	}
}

char *string::data()
{
	return inlined() ? data_inline_contents_ : data_;
}

bool string::inlined() const
{
	return data_inline_length_ != (unsigned char)-1;
}

void string::resize(int new_length)
{
	const char *old_data = data();

	if(new_length > max_inline_length_ && (inlined() || size_ < new_length)){ //SSO or big to big
		int new_size = bit_round(new_length + 1);
		data_ = copy(old_data, min(length(), new_length), new char[new_size]);
		size_ = new_size;
	}else if(length() >= max_inline_length_){ //big to SSO
		copy(old_data, new_length, data_inline_contents_);
	}
	set_length(new_length);
	
	if(old_data != data_inline_contents_ && old_data != data()){
		delete []old_data;
	}

	data()[new_length] = 0; //always ensure null terminator
}

void string::set_length(int length)
{
	if(length > max_inline_length_){
		data_inline_length_ = (unsigned char)-1;
		length_ = length;
	}else{
		data_inline_length_ = length;
	}
}

void string::copy_string(const string &other)
{
	resize(other.length());
	copy(other.raw(), length(), data());
}

void string::copy_cstring(const char *cstring, int size)
{
	resize(size);
	copy(cstring, length(), data());
}


void test_string()
{
	string test1 = "test";
	string test2 = "testtesttesttest";
	assert(!strcmp(test1.raw(), "test"));
	assert(!strcmp(test2.raw(), "testtesttesttest"));
	assert(test1[2] == 's');
	
	string test3 = std::move(test2);
	assert(!strcmp(test2.raw(), ""));
	assert(!strcmp(test3.raw(), "testtesttesttest"));
	
	string test4 = std::move(test1);
	assert(!strcmp(test1.raw(), "test"));
	assert(!strcmp(test4.raw(), "test"));
	
	string test5 = test3;
	assert(!strcmp(test5.raw(), "testtesttesttest"));
	
	string test6 = test1;
	assert(!strcmp(test6.raw(), "test"));
	
	string test7;
	assert(!strcmp(test7.raw(), ""));
	
	//operators
	assert(test1 == test4);
	assert(test1 != test2);
	assert(test2 == test2);
	test1 += test4;
	assert(!strcmp(test1.raw(), "testtest"));
	test1 += test5;
	assert(!strcmp(test1.raw(), "testtesttesttesttesttest"));
	test2 += test1;
	assert(!strcmp(test2.raw(), "testtesttesttesttesttest"));
	
	//methods
	string test8 = "                test \n";
	assert(!strcmp(test8.trim().raw(), "test"));
}

