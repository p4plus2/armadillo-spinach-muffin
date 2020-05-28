#pragma once
#include <cassert>

template<typename E, typename... R>
struct tuple : public tuple<R...>{
	tuple(){}
	tuple(E element, R... remaining) : tuple<R...>(remaining...), element(element) { size = sizeof...(R) + 1; }
	tuple(const tuple<E, R...> &other) : tuple<R...>(other), element(other.element) { size = sizeof...(R) + 1; }
	E element;
	int size;
};

template<typename E>
struct tuple<E>{
	tuple(){}
	tuple(E element): element(element) {}
	tuple(const tuple<E> &other): element(other.element) {}
	E element;
	int size = 1;
};

namespace detail{
	template<int index, typename E, typename... R>
	struct tuple_get{
		static auto value(const tuple<E, R...>* t)
		{
			return tuple_get<index - 1, R...>::value(t);
		}
	};

	template<typename E, typename... R>
	struct tuple_get<0, E, R...> {
		static E value(const tuple<E, R...>* t)
		{
			return t->element;
		}
	};
}

template<int index, typename E, typename... R>
auto get(const tuple<E, R...>& t)
{
	return detail::tuple_get<index, E, R...>::value(&t);
}

namespace detail{
	template <int I>
	struct visit_tuple{
		template <typename T, typename F>
		static void visit(T& target, int i, F call)
		{
			if (i == I - 1){
				call(get<I - 1>(target));
			}else{
				visit_tuple<I - 1>::visit(target, i, call);
			}
		}
	};

	template <>
	struct visit_tuple<0>{
		template <typename T, typename F>
		static void visit(T&, int, F)
		{
			assert(false);
		}
	};
}


template <typename F, typename... T>
void visit(const tuple<T...> &target, int i, F call)
{
	detail::visit_tuple<sizeof...(T)>::visit(target, i, call);
}
