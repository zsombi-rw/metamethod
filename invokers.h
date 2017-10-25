#ifndef INVOKERS_H
#define INVOKERS_H

#include <functional>
#include <utility>  // index_sequence

using namespace std;

namespace tuple_invoke
{

namespace impl
{
template <class Class, typename Func, typename Tuple, size_t... Indices>
constexpr auto apply(Class&& o, Func&& f, Tuple&& t, index_sequence<Indices...>)
{
    return(forward<Class>(o)->*forward<Func>(f))(get<Indices>(forward<Tuple>(t))...);
}
} // namespace impl

template <class Func, class Class, class Tuple>
constexpr auto apply(Func&& f, Class&& o, Tuple&& t)
{
    return impl::apply(
        forward<Class>(o), forward<Func>(f), forward<Tuple>(t),
        make_index_sequence<tuple_size<decay_t<Tuple>>::value>{});
}
} // namespace tuple_invoke


#endif // INVOKERS_H
