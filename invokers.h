#ifndef INVOKERS_H
#define INVOKERS_H

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////
/// expand the tuple and invoke
///
namespace tuple_invoke
{

template<size_t N>
struct ApplyMember
{
    template<class Class, typename Func, typename Tuple, typename... Arguments>
    static inline auto apply(Class&& c, Func&& f, Tuple&& t, Arguments&&... a) ->
        decltype(ApplyMember<N-1>::apply(forward<Class>(c), forward<Func>(f), forward<Tuple>(t), get<N-1>(forward<Tuple>(t)), forward<Arguments...>(a)...))
    {
        return ApplyMember<N-1>::apply(forward<Class>(c), forward<Func>(f), forward<Tuple>(t), get<N-1>(forward<Tuple>(t)), forward<Arguments...>(a)...);
    }
};

// call the method
template<>
struct ApplyMember<0>
{
    template<class Class, typename Func, typename Tuple, typename... Arguments>
    static inline auto apply(Class&& c, Func&& f, Tuple&&, Arguments&&... a) ->
        decltype((forward<Class>(c)->*forward<Func>(f))(forward<Arguments>(a)...))
    {
        return(forward<Class>(c)->*forward<Func>(f))(forward<Arguments>(a)...);
    }
};

// the main function
template<class Class, typename Func, typename Tuple>
inline auto apply(Class&& c, Func&& f, Tuple&& t) ->
    decltype(ApplyMember<tuple_size<typename decay<Tuple>::type>::value>::apply(forward<Class>(c), forward<Func>(f), forward<Tuple>(t)))
{
    return ApplyMember<tuple_size<typename decay<Tuple>::type>::value>::apply(forward<Class>(c), forward<Func>(f), forward<Tuple>(t));
}

} // namespace tuple_invoke

#endif // INVOKERS_H
