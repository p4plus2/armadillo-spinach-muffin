#pragma once

#include "traits.h"
#include "tuple.h"
#include "string.h"
#include "string_view.h"
#include <cstdio>
#include <cctype>
#include <cassert>

namespace fmt{
	enum error_state{
		NONE,
		INVALID_PRECISION_FORMATTING,
		INVALID_ESCAPE_SEQUENCE,
		INVALID_ARGUMENT_SEPERATOR,
		INVALID_FORMAT_CHARACTER
	};
	
	static thread_local error_state error;
}

namespace detail
{
	template<typename T>
	string dump_bytes(const T &t)
	{
		string output;
		
		for(auto i = 0u; i < sizeof(T); i++){
			output.append(to_string(*((char *)&t + i), 16));
			if(i < sizeof(T) - 1){
				output.append(' ');
			}
		}
		
		return output;
	}
	
	template<typename T> using has_to_string = decltype(declval<T&>().to_string());
	template<typename T> using has_to_string_free = decltype(to_string(declval<T&>()));

	template<typename T>
	string convert(T const& t)
	{
		if constexpr(is_detected_exact_v<string, has_to_string, T>){
			return t.to_string();
		}else if constexpr(is_detected_exact_v<string, has_to_string_free, T> && !is_enum_v<T>){
			return to_string(t);
		}else if constexpr(is_static_castable_v<T, string>){
			return static_cast<string>(t);
		}else if constexpr(is_enum_v<T>){	//enum without a to_string override
			return to_string((int)t);
		}
		
		//give up no valid conversion exists, so dump the bytes and maybe that helps debug
		return dump_bytes(t);
	}
	
	enum parse_state{
		READ,
		ESCAPE,
		FORMAT_OPEN,
		FORMAT_CLOSE,
		
		READ_FILL,
		READ_SEPERATOR,
		READ_ARGUMENT,
		READ_PRECISION
	};
	
	enum format_padding{
		NONE,
		LEFT,
		RIGHT
	};
	
	enum format_base{
		BASE_2,
		BASE_10,
		BASE_16
	};
	
	struct format_specifier{
		char fill = ' ';
		format_padding padding = NONE;
		format_base base = BASE_10;
		int length = 0;
		bool prefix = false;
		int precision = 0;
	};
	
	static format_specifier parse_format_string(const string &format_string)
	{
		string_view view = format_string;
		format_specifier specifier;
		parse_state state = READ_FILL;
		fmt::error = fmt::NONE;
		
		while(view.length()){
			if(state == READ_FILL){
				if(view.read_if(',')){
					state = READ_ARGUMENT;
				}else{
					specifier.fill = view.read();
					state = READ_SEPERATOR;
				}
			}else if(state == READ_SEPERATOR){
				char c = view.read();
				state = (c == ',') ? READ_ARGUMENT : state;
				state = (c == '.') ? READ_PRECISION : state;
					
				if(c != ',' && c != '.'){
					fmt::error = fmt::INVALID_ARGUMENT_SEPERATOR;
				}
				
			}else if(state == READ_ARGUMENT){
				if(view.peek() == '-' || isdigit(view.peek())){
					if(specifier.length != 0){
						fmt::error = fmt::INVALID_FORMAT_CHARACTER;
					}
					
					specifier.length = view.read_int();
					specifier.padding = specifier.length < 0 ? RIGHT : LEFT;
				}else{
					switch(view.read()){
						case 'b':
							specifier.base = BASE_2;
						break;
						case 'x':
							specifier.base = BASE_16;
						break;
						case '#':
							specifier.prefix = true;
						break;
						default:
							fmt::error = fmt::INVALID_FORMAT_CHARACTER;
						break;
					}
				}
			}else if(state == READ_PRECISION){
				if(isdigit(view.peek())){
					specifier.precision = view.read_int();
				}else{
					fmt::error = fmt::INVALID_PRECISION_FORMATTING;
				}
			}
			
			if(fmt::error != fmt::NONE){
				break;
			}
		}
		
		return specifier;
	}
}

inline fmt::error_state print_error()
{
	return fmt::error;
}

template<typename T>
string format(const string &format_string, const T &t)
{
	string formatted;
	detail::format_specifier specifier = detail::parse_format_string(format_string);
	
	if constexpr(is_integral_v<T>){
		if(specifier.prefix){
			if(specifier.base == detail::BASE_16){
				formatted = "0x";
			}else if(specifier.base == detail::BASE_2){
				formatted = "0b";
			}
		}
		int base_lookup[] = {2, 10, 16};
		formatted.append(to_string(t, base_lookup[specifier.base]));
	}else if constexpr(is_floating_point_v<T>){
		formatted = to_string(t, specifier.precision);
	}else{
		formatted = detail::convert(t);
	}
	
	if(specifier.padding == detail::LEFT){
		formatted.left_pad(specifier.fill, specifier.length);
	}else if(specifier.padding == detail::RIGHT){
		formatted.right_pad(specifier.fill, specifier.length);
	}
	
	return formatted;
}

template<typename... A>
void echo(const A ...args)
{
    (fputs(detail::convert(args).raw(), stdout), ...);
}

template<typename T, typename... A>
void print(const T &t, const A ...args)
{
	echo(t);
	if constexpr(sizeof...(args)){
		putc(' ', stdout);
		print(args...);
	}
}

template<typename... A>
void print_ln(const A ...args)
{
	if constexpr(sizeof...(args)){
		print(args...);
	}
	putc('\n', stdout);
}
//i'd much rather call this printf but that causes namespace issues
inline void print_f(const string &format_string)
{
	echo(format_string); //what you meant to say...
}

template<typename... A>
void print_f(const string &format_string, const A ...args)
{
	tuple arg_collection = {args...};
	auto state = detail::READ;
	auto arg_count = 0;
	string format_arguments;
	
	for(auto c : format_string){
		if(state == detail::ESCAPE){
			if(c == '{' || c == '%'){
				putc(c, stdout);
			}else{
				fmt::error = fmt::INVALID_ESCAPE_SEQUENCE;
			}
			state = detail::READ;
		}
		
		if(state == detail::FORMAT_OPEN && c != '}'){
			format_arguments.append(c);
			continue;
		}
		
		switch(c){
			case '\\':
				state = detail::ESCAPE;
			break;
			case '%':
				visit(arg_collection, arg_count, [](auto value){ 
					echo(value);
				});
				arg_count++;
			break;
			case '{':
				state = detail::FORMAT_OPEN;
			break;
			case '}':
				if(state == detail::FORMAT_OPEN){
					visit(arg_collection, arg_count, [&format_arguments](auto value){ 
						echo(format(format_arguments, value)); 
					});
					arg_count++;
					state = detail::READ;
				}else{
					putc(c, stdout);
				}
			break;
			default:
				putc(c, stdout);
		}
	}
}

#define assertf(A, M, ...) if(!(A)){ print_f("Error: " M "\n", ##__VA_ARGS__); assert(A); }
