#include <iostream>
#include "metaclass.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////
///
///
class Object : public MetaObject
{
    METACLASS_BEGIN(Object, MetaObject)
        META_METHOD(voidFunc, void)
        META_METHOD(intRetFunc, int)
        META_METHOD(intArgFunc, void, int)
        META_METHOD(intRetArgFunc, int, int)
        META_METHOD(intRetVectorFunc, size_t, const vector<int>&)
        META_METHOD(intRetVectorFunc, size_t, int, const vector<int>&)
        META_METHOD(intRetVectorFunc2, int, int, const vector<int>&)
        META_METHOD(voidStringFunc, void, const string&)
        META_METHOD(voidCStringFunc, void, const char*)
        META_METHOD(abstractMethod, int, const vector<int>&)
    METACLASS_END()
public:
    template<class TObject>
    static TObject *create()
    {
        TObject *object = new TObject;
        object->initMetaClass(object);
        return object;
    }
    virtual ~Object() {}

    void voidFunc() { cout << "voidFunc called" << endl; }
    int intRetFunc() { return 100; }
    void intArgFunc(int arg) { cout << "argument: " << arg << endl; }
    int intRetArgFunc(int arg) { return arg * 10; }
    virtual size_t intRetVectorFunc(const vector<int> &v) { return v.size(); }
    size_t intRetVectorFunc(int, const vector<int> &v) { return v.size(); }
    int intRetVectorFunc2(int i, const vector<int> &v) { return i * int(v.size()); }
    void voidStringFunc(const string &s) { cout << "STRING: " << s << endl; }
    void voidCStringFunc(const char *s) { cout << "CSTRING: " << s << endl; }
    int abstractMethod(const vector<int>& v) override { return 100 * v.size(); }

protected:
    explicit Object() {}
};
METAOBJECT(Object, MetaObject)

class Derived : public Object
{
    METACLASS_BEGIN(Derived, Object)
        META_METHOD(intRetVectorFunc, size_t, const vector<int>&)
        META_METHOD(abstractMethod, int, const vector<int>&)
    METACLASS_END()
public:
    explicit Derived() {}

    size_t intRetVectorFunc(const vector<int> &v) override { return 2 * v.size(); }
    int abstractMethod(const vector<int>& v) override { return 200 * int(v.size()); }
};
METAOBJECT(Derived, Object)


//////////////////////////////////////////////////////////////////////////////////////
///
///
#define VERIFY(a)   if (!(a)) cerr << #a << " FAILED" << endl
#define COMPARE(a, e)   if ((a) != (e)) cerr << "FAILED" << endl << "Actual: " << a  << endl << "Expected: " << e << endl
int main()
{
    unique_ptr<Object> object(Object::create<Object>());

    VERIFY(MetaClass::invoke<void>(object.get(), "voidFunc"));
    // eat arguments
    VERIFY(!MetaClass::invoke<void>(object.get(), "voidFunc", 10));
    // return value
    int ret = -1;
    VERIFY(MetaClass::invoke<int>(object.get(), ret, "intRetFunc"));
    COMPARE(ret, 100);

    ret = -1;
    VERIFY(!MetaClass::invoke<int>(object.get(), ret, "intRetFunc", -20));
    COMPARE(ret, -1);

    VERIFY(MetaClass::invoke<void>(object.get(), "intArgFunc", 10));
    VERIFY(!MetaClass::invoke<void>(object.get(), "intArgFunc", "monkey"));
    VERIFY(MetaClass::invoke<int>(object.get(), ret, "intRetArgFunc", 5));
    COMPARE(ret, 50);

    vector<int> v(10);
    size_t uret = 0;
    VERIFY(MetaClass::invoke<size_t>(object.get(), uret, "intRetVectorFunc", v));
    COMPARE(uret, 10);

    uret = 0;
    vector<std::string> s(20);
    VERIFY(!MetaClass::invoke<size_t>(object.get(), uret, "intRetVectorFunc", s));
    COMPARE(uret, 0);

    VERIFY(MetaClass::invoke<void>(object.get(), "voidStringFunc", string("foo")));
    bool b = MetaClass::invoke<void, const string&>(object.get(), "voidStringFunc", "foo");
    VERIFY(b);
    VERIFY(!MetaClass::invoke<void>(object.get(), "voidStringFunc", "foo"));
    VERIFY(MetaClass::invoke<void>(object.get(), "voidCStringFunc", "foo"));

    unique_ptr<Derived> o2(Derived::create<Derived>());
    ret = -1;
    VERIFY(MetaClass::invoke<int>(o2.get(), ret, "abstractMethod", v));
    COMPARE(ret, 2000);

    ret = -1;
    VERIFY(MetaClass::invoke<int>(object.get(), ret, "abstractMethod", v));
    COMPARE(ret, 1000);

    COMPARE(object->abstractMethod(v), 1000);

    // invoke via polymorphism
    MetaObject *metaObject = o2.get();
    ret = -1;
    VERIFY(MetaClass::invoke<int>(metaObject, ret, "abstractMethod", v));
    COMPARE(ret, 2000);

    VERIFY(MetaClass::invoke<void>(o2.get(), "voidFunc"));
    VERIFY(MetaClass::invoke<void>(metaObject, "voidFunc"));

    MetaClass::MetaMethodRange range = MetaClass::methodRange(object.get(), "intRetVectorFunc");
    for (MetaClass::MetaMethodIterator i = range.first; i != range.second; ++i) {
        cout << "method " << i->second->argumentCount() << endl;
        for (arguments::ArgIterator j = i->second->argumentsBegin(); j != i->second->argumentsEnd(); ++j) {
            cout << "  arg " << j->m_type.name() << endl;
        }
    }

    // tuple_invoke
    {
        arguments::ArgContainer args = {
            arguments::ArgumentType::value<int>(),
            arguments::ArgumentType::value<vector<int>>()
        };
        const MetaClass *mo = object->metaObject();
        const MetaMethodBase *method = mo->getMethod<int>("intRetVectorFunc", args);
        (void)(method);
        auto t = make_tuple(12, vector<int>({1, 2, 3, 4}));
        Object *o = object.get();
        cout << "TUPLECALL " << tuple_invoke::apply(&Object::intRetVectorFunc2, o, t) << endl;
//        method->apply<Object>(object.get(), &Object::intRetVectorFunc, t);
    }

    // dynamic invoke
    VERIFY(MetaClass::invoke(object.get(), "voidFunc"));
    return 0;
}
