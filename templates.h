#ifndef TEMPLATES_H
#define TEMPLATES_H
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
    }
}

#endif