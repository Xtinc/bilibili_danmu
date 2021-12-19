#ifndef TEMPLATES_H
#define TEMPLATES_H

#include <utility>
#include <functional>
#include <algorithm>
#include <initializer_list>

namespace danmu
{
    namespace details
    {
        template <typename S, typename T, typename C>
        S findvalue_t(S &start, const T &str, C ch)
        {
            return str.find(ch, start);
        }
        template <typename S, typename T, typename C, typename... TP>
        S findvalue_t(S &start, const T &str, C fch, TP... chs)
        {
            start = str.find(fch, start);
            if (start == std::string::npos)
            {
                throw std::logic_error{"Keywords not found."};
            }
            start += strlen(fch);
            return findvalue_t(start, str, chs...);
        }
        template <typename Ret, typename Fun, typename Key>
        Ret call_it(Fun &fun, const Key &key, char (*)[sizeof(fun(key))] = 0)
        {
            return fun(key);
        }
        template <typename Ret, typename Fun>
        Ret call_it(Fun &fun, ...)
        {
            return fun();
        }
    }
    template <typename KeyType, typename Signature, typename Comp = std::less<KeyType>>
    typename std::function<Signature>::result_type
    Switch(const KeyType &value, std::initializer_list<std::pair<const KeyType, std::function<Signature>>> sws, Comp comp = Comp())
    {
        typedef std::pair<const KeyType &, std::function<Signature>> KVT;
        typedef typename std::function<Signature>::result_type RetType;
        auto cmp = [&comp](const KVT &a, const KVT &b)
        { return comp(a.first, b.first); };
        auto val = KVT(value, std::function<Signature>());
        auto r = std::lower_bound(sws.begin(), sws.end(), val, cmp);
        if ((r != sws.end()) && (!cmp(val, *r)))
        {
            return detail::call_it<RetType>(r->second, r->first);
        }
        return RetType();
    }

    template <typename KeyType, typename FunPtrType, typename Comp = std::less<KeyType>>
    typename std::enable_if<std::is_pointer<FunPtrType>::value, void>::type
    Switch(const KeyType &value, std::initializer_list<std::pair<const KeyType, FunPtrType>> sws, Comp comp = Comp())
    {
        typedef std::pair<const KeyType &, FunPtrType> KVT;
        auto cmp = [&comp](const KVT &a, const KVT &b)
        { return comp(a.first, b.first); };
        auto val = KVT(value, FunPtrType());
        auto r = std::lower_bound(sws.begin(), sws.end(), val, cmp);
        if ((r != sws.end()) && (!cmp(val, *r)))
        {
            detail::call_it<void>(r->second, r->first);
        } // else: not found
    }
    // (c) 2012 Tobias Hoffmann, Dual-licensed: MIT/LGPL
    /* Usage:

    1)
      Switch<int,void (*)(int)>(1000,{ // sorted:
        {35,[](int x){ printf("0 %d\n",x); }},
        {234,[](int x){ printf("2 %d\n",x); }},
        {645,[](int x){ printf("3 %d\n",x); }},
        {909,[](int x){ printf("4 %d\n",x); }},
        {1000,[](int x){ printf("5 %d\n",x); }}
      });

    2)
      Switch<const char *,void (*)()>("asdf",{ // sorted:
        {"asdf",[]{ printf("0\n"); }},
        {"bde",[]{ printf("1\n"); }},
        {"ger",[]{ printf("2\n"); }}
      },[](const char *a,const char *b){ return strcmp(a,b)<0;});

    3)
      YourType k;
      auto ret=Switch<int,int(int)>(123,{
        {234,[](int x){ printf("0 %d\n",x); return 14; }}
        {123,[&k](int x){ printf("1 %d\n",x); k=...; return 21; }},
        {35,[](int x){ printf("2 %d\n",x); return 23; }},
      },std::greater<int>());
      printf("%d\n",ret); // int() if not matched

    4)
      #include <boost/optional.hpp>
      auto ret=Switch<int,boost::optional<int>(int)>(133,{
        {35,[](int x){ printf("0 %d\n",x); return 23; }},
        {123,[&x](int x){ printf("1 %d\n",x); return 21; }},
        {234,[](int x){ printf("2 %d\n",x); return 14; }}
      });
      if (ret) {
        printf("%d\n",*ret);
      } else {
        printf("not found\n");
      }

    */
}

#endif