#ifndef METACLASS_H
#define METACLASS_H

#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <typeindex>

#include "metatype.h"
#include "function_traits.h"
#include "arguments.h"
#include "invokers.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////
///
///
class MetaObject;
namespace metainvoker
{

typedef void (*Invoker)(MetaObject*, ReturnArgumentBase ret, vector<ArgumentBase>);

template <class TClass, typename Ret, Ret (TClass::*Fun)()>
void invoker (MetaObject *object, ReturnArgumentBase ret, vector<ArgumentBase>)
{
    (static_cast<TClass*>(object)->*Fun)();
}

template <class TClass, typename Ret, typename T1, Ret (TClass::*Fun)(T1)>
void invoker (MetaObject *object, ReturnArgumentBase ret, vector<ArgumentBase>)
{
}

template <class TClass, typename Ret, typename T1, typename T2, Ret (TClass::*Fun)(T1, T2)>
void invoker (MetaObject *object, ReturnArgumentBase ret, vector<ArgumentBase>)
{

}


} // namespace metainvoker
//////////////////////////////////////////////////////////////////////////////////////
///
///
class MetaObject;
class MetaMethodBase
{
public:

    virtual ~MetaMethodBase() {}
    explicit MetaMethodBase(const string &name)
        : m_name(name)
    {
    }

    string name() const
    {
        return m_name;
    }

    arguments::ArgIterator argumentsBegin() const
    {
        return ++m_arguments.begin();
    }
    arguments::ArgIterator argumentsEnd() const
    {
        return m_arguments.end();
    }

    int argumentCount() const
    {
        return m_arguments.size() - 1;
    }

    bool isReturnType(const arguments::ArgumentType &retType)
    {
        return (m_arguments[0] == retType);
    }

    bool compatibleArguments(const arguments::ArgContainer &invokeArgs)
    {
        if (m_arguments.size() - 1 == invokeArgs.size()) {
            arguments::ArgIterator argsThis = ++m_arguments.cbegin();
            arguments::ArgIterator argsThat = invokeArgs.cbegin();
            bool ok = true;
            while (ok && (argsThis != m_arguments.cend()) && (argsThat != invokeArgs.cend())) {
                ok = argsThis->isCompatible((*argsThat));
                ++argsThat;
                ++argsThis;
            }
            return ok;
        }
        return false;
    }

    template <class Class, typename Func, typename Tuple>
    inline bool apply(Class && c, Func && f, Tuple && t)
    {
        if (traits::function_traits<typename decay<Func>::type>::arity == m_arguments.size() - 1) {
            tuple_invoke::apply(forward<Class>(c), forward<Func>(f), forward<Tuple>(t));
            return true;
        }
        return false;
    }

    virtual bool invoke(MetaObject *object, ReturnArgumentBase ret, vector<ArgumentBase> &args) = 0;

protected:
    string m_name;
    arguments::ArgContainer m_arguments;
};

//////////////////////////////////////////////////////////////////////////////////////
///
///
template <class TObject, typename TReturnType, typename... Arguments>
class MetaMethod : public MetaMethodBase
{
    TReturnType (TObject::*m_method)(Arguments...);
    metainvoker::Invoker m_invoker = nullptr;
public:

    explicit MetaMethod(TReturnType (TObject::*method)(Arguments...), metainvoker::Invoker invoker, const string &name)
        : MetaMethodBase(name)
        , m_invoker(invoker)
        , m_method(method)
    {
        m_arguments = arguments::argumentTypes(method);
    }
    virtual ~MetaMethod() {}

    inline TReturnType invoke(TObject *object, Arguments... args)
    {
        if (std::is_same<TReturnType, void>::value) {
            (object->*m_method)(args...);
        } else {
            return (object->*m_method)(args...);
        }
    }

    template<typename Tuple>
    inline auto apply(TObject *object, Tuple&& t)
    {
        if (std::is_same<TReturnType, void>::value) {
            tuple_invoke::apply(m_method, object, forward<Tuple>(t));
        } else {
            return tuple_invoke::apply(m_method, object, forward<Tuple>(t));
        }
    }

    bool invoke(MetaObject *object, ReturnArgumentBase ret, vector<ArgumentBase> &args) override
    {
        if (args.size() != argumentCount()) {
            return false;
        }
        // check if the return type is similar to what we need
        if (ret.isValid() && !isReturnType(ret.type())) {
            return false;
        }

        // invoke the method
        if (m_invoker) {
            m_invoker(object, ret, args);
        }

        return true;
    }
};

//////////////////////////////////////////////////////////////////////////////////////
///
///
class MetaObject;
class MetaClass
{
    typedef multimap<string, MetaMethodBase*> MetaMethodContainer;
    const MetaClass *m_superClass = nullptr;
    MetaMethodContainer m_methods;

public:
    explicit MetaClass(const MetaClass *super = nullptr)
        : m_superClass(super)
    {
    }
    ~MetaClass()
    {
        for (auto mm : m_methods) {
            delete mm.second;
        }
    }
    void addMetaMethod(MetaMethodBase *method)
    {
        m_methods.insert(pair<string, MetaMethodBase*>(method->name(), method));
    }

    typedef MetaMethodContainer::const_iterator MetaMethodIterator;
    typedef pair<MetaMethodIterator, MetaMethodIterator> MetaMethodRange;
    static MetaMethodRange methodRange(MetaObject *object, const string &name);

    const MetaClass *superClass() const
    {
        return m_superClass;
    }

    template<typename TReturnType, typename... Arguments>
    static bool invoke(MetaObject *o, const string &signature, Arguments... args);
    template<typename TReturnType, typename... Arguments>
    static bool invoke(MetaObject *o, TReturnType &ret, const string &signature, Arguments... args);

    static bool invoke(MetaObject *object, const string &name,
                       ReturnArgumentBase ret = ReturnArgumentBase(),
                       vector<ArgumentBase> args = vector<ArgumentBase>());

    template<class TObject, typename Tuple>
    static bool apply(TObject *object, const string &name, Tuple&& arguments)
    {
        const MetaClass *mo = object->metaClass();
        MetaMethodRange range = mo->m_methods.equal_range(name);
        if (range.first == range.second) {
            return false;
        }
        for (MetaMethodIterator i = range.first; i != range.second; ++i) {
            size_t argCount = tuple_size<typename decay<Tuple>::type>::value;
            if (i->second->argumentCount() == argCount) {
//                tuple_invoke::apply(object, i->second->m_method, forward<Tuple>(arguments));
                return true;
            }
        }
        return false;
    }

    template<typename TReturnType>
    MetaMethodBase *getMethod(const string &name, const arguments::ArgContainer &argTypes) const
    {
        arguments::ArgumentType returnType = arguments::ArgumentType::value<TReturnType>();
        for (auto mm : m_methods) {
            if ((mm.first == name) &&
                mm.second->isReturnType(returnType) &&
                mm.second->compatibleArguments(argTypes)) {
                return mm.second;
            }
        }
        return nullptr;
    }
};

#if defined(__clang__)
    #define WARNING_PUSH                _Pragma("clang diagnostic push")
    #define DISABLE_OVERRIDE_WARNING    _Pragma("clang diagnostic ignored \"-Winconsistent-missing-override\"")
    #define WARNING_POP                 _Pragma("clang diagnostic pop")
#else
    #define WARNING_PUSH
    #define DISABLE_OVERRIDE_WARNING
    #define WARNING_POP
#endif

#define METACLASS_BEGIN(Class, SuperClass) \
    public: \
    static const MetaClass staticMetaObject; \
    WARNING_PUSH \
    DISABLE_OVERRIDE_WARNING \
    virtual const MetaClass *metaObject() const { return &staticMetaObject; } \
    virtual void initMetaClass(Class *thisClass) \
    { \
        typedef class Class TClass; \
        SuperClass::initMetaClass(static_cast<SuperClass*>(thisClass)); \
        MetaClass *mo = const_cast<MetaClass*>(thisClass->metaObject()); \
        (void)(mo);

#define METACLASS_END() \
    } \
    WARNING_POP

#define METAOBJECT(Class, SuperClass) \
const MetaClass Class::staticMetaObject { &SuperClass::staticMetaObject };

#define META_METHOD(Method, ReturnType, ...) \
    mo->addMetaMethod(new MetaMethod<TClass, ReturnType, ##__VA_ARGS__>( \
        &TClass::Method, \
        &metainvoker::invoker<TClass, ReturnType, ##__VA_ARGS__, &TClass::Method>, \
        #Method));

//////////////////////////////////////////////////////////////////////////////////////
///
///
class MetaObject
{
public:
    // metadata section for the default class
    static const MetaClass staticMetaObject;
    virtual const MetaClass *metaObject() const { return &staticMetaObject; }
    virtual void initMetaClass(MetaObject *)
    {
        typedef class MetaObject TClass;
        MetaClass *mo = const_cast<MetaClass*>(metaObject());
        META_METHOD(abstractMethod, int, const vector<int>&)
    }

public:
    explicit MetaObject() {}
    virtual ~MetaObject() {}

    virtual int abstractMethod(const vector<int> &) = 0;
};
const MetaClass MetaObject::staticMetaObject;

//////////////////////////////////////////////////////////////////////////////////////
///
///
MetaClass::MetaMethodRange MetaClass::methodRange(MetaObject *object, const string &name)
{
    return object->metaObject()->m_methods.equal_range(name);
}

template<typename TReturnType, typename... Arguments>
bool MetaClass::invoke(MetaObject *o, const string &signature, Arguments... args)
{
    arguments::ArgContainer argTypes = arguments::argumentTypes(forward<Arguments>(args)...);
    MetaClass *mo = const_cast<MetaClass*>(o->metaObject());
    while (mo) {
        MetaMethodBase *method = mo->getMethod<TReturnType>(signature, argTypes);
        if (method) {
            MetaMethod<MetaObject, TReturnType, Arguments...> *mCasted =
                    static_cast<MetaMethod<MetaObject, TReturnType, Arguments...>*>(method);
            if (mCasted) {
                mCasted->invoke(o, args...);
                return true;
            }
        }
        // check superclass
        mo = const_cast<MetaClass*>(mo->superClass());
    }
    return false;
}

template<typename TReturnType, typename... Arguments>
bool MetaClass::invoke(MetaObject *o, TReturnType &ret, const string &signature, Arguments... args)
{
    arguments::ArgContainer argTypes = arguments::argumentTypes(forward<Arguments>(args)...);
    MetaClass *mo = const_cast<MetaClass*>(o->metaObject());
    while (mo) {
        MetaMethodBase *method = o->metaObject()->getMethod<TReturnType>(signature, argTypes);
        if (method) {
            MetaMethod<MetaObject, TReturnType, Arguments...> *mCasted =
                    static_cast<MetaMethod<MetaObject, TReturnType, Arguments...>*>(method);
            if (mCasted) {
                if (is_void<TReturnType>::value) {
                    mCasted->invoke(o, args...);
                } else {
                    ret = mCasted->invoke(o, args...);
                }
                return true;
            }
        }
        // continue in superclass
        mo = const_cast<MetaClass*>(mo->superClass());
    }
    return false;
}

bool MetaClass::invoke(MetaObject *object, const string &name,
                   ReturnArgumentBase ret, vector<ArgumentBase> args)
{
    if (args.size() > MAX_ARGS) {
        return false;
    }
    MetaClass *mo = const_cast<MetaClass*>(object->metaObject());
    while (mo) {
        MetaMethodRange range = mo->methodRange(object, name);
        for (MetaMethodIterator i = range.first; i != range.second; ++i) {
            if (args.size() == i->second->argumentCount()) {
                // call the invoker
                if (i->second->invoke(object, ret, args)) {
                    return true;
                }
            }
        }
        // continue in superclass
        mo = const_cast<MetaClass*>(mo->superClass());
    }
    return false;
}

#endif // METACLASS_H
