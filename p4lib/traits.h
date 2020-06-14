#pragma once

#include <type_traits>
#include <initializer_list>
#include "tuple.h"

using std::declval;
using std::enable_if_t;
using std::true_type;
using std::false_type;
using std::is_integral_v;
using std::is_enum_v;
using std::is_signed_v;
using std::is_floating_point_v;
using std::is_same_v;
using std::is_trivially_copyable_v;
using std::is_base_of_v;
using std::is_bounded_array_v;
using std::remove_reference;
using std::remove_cv;
using std::decay;

using std::initializer_list;

//a flag to mark a type as sort() compatible.
//said types should be able to return data() and length()
class sortable{};

//a flag to mark a type as having standard manipulation
//said types should be able to use remove, operator[],
//insert, slice, and append
class container{};

//a flag to mark a type as having a begin and end
//which can be incremented in some way to access
//each element, but order may not be guarenteed
class iterable{};


template <typename T> constexpr 
typename remove_reference<T>::type&& move(T&& arg)
{
	return static_cast<typename remove_reference<T>::type&&>(arg);
}

template <class T> constexpr 
inline T&& forward(typename std::remove_reference<T>::type& t) noexcept
{
	return static_cast<T&&>(t);
}

template <class T> constexpr 
inline T&& forward(typename std::remove_reference<T>::type&& t) noexcept
{
	return static_cast<T&&>(t);
}


namespace detail
{
	struct unconstructable{
		unconstructable() = delete;
		~unconstructable() = delete;
		unconstructable(unconstructable const&) = delete;
		void operator=(unconstructable const&) = delete;
	};

	template <typename D, typename V, template <typename...> typename O, typename... A>
	struct detector{
		using value_t = false_type;
		using type = D;
	};

	template <class D, template <typename...> typename O, typename... A>
	struct detector<D, std::void_t<O<A...>>, O, A...>{
		using value_t = true_type;
		using type = O<A...>;
	};
	    
	template <template<typename...> typename O, typename... A>
	using detected_t = typename detector<unconstructable, void, O, A...>::type;
	
	template <typename F, typename T, typename = T>
	struct is_static_castable : false_type{};

	template <typename F, typename T>
	struct is_static_castable<F, T, decltype(static_cast<T>(declval<F>()))> : true_type{};
	
	
	template <typename T, template <typename...> typename S>
	struct is_specialization_of : false_type{};
	template <template <typename...> typename S, typename... A>
	struct is_specialization_of<S<A...>, S> : true_type{};
}

template <typename E, template<typename...> typename O, typename... A>
constexpr bool is_detected_exact_v = is_same_v<E, detail::detected_t<O, A...>>;

template <typename F, typename T>
constexpr bool is_static_castable_v = detail::is_static_castable<F, T>::value;

template <typename T>
constexpr bool is_tuple_v = detail::is_specialization_of<T, tuple>::value;

