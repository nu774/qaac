#ifndef EXPAND_H
#define EXPAND_H
#include <vector>
#include <string>
#include <streambuf>
#include <sstream>
#include <algorithm>
#include <iterator>

/*
 * - ${VAR} evaluates to variable VAR's value
 * - ${VAR&word} evaluates to ${VAR} if ${VAR} is empty,
 *   otherwise word
 * - ${VAR|word} evaluates to ${VAR} if ${VAR} is not empty,
 *   otherwise word
 */

template <typename CharT, typename Iterator, typename Func>
std::basic_string<CharT>
process1(Iterator &begin, Iterator end, Func lookup, int delim);

template <typename CharT, typename Iterator, typename Func>
std::basic_string<CharT> expand(Iterator &begin, Iterator end, Func lookup)
{
    std::basic_string<CharT> name, value, value2;
    CharT c;
    while (begin < end && (c = *begin++) != '&' && c != '|' && c != '}')
        name.push_back(c);
    value = lookup(name);
    if (c != '&' && c != '|')
        return value;
    // XXX: not short-circuit
    value2 = process1<CharT>(begin, end, lookup, '}');
    if (c == '&')
        return value.empty() ? value : value2;
    else
        return value.empty() ? value2 : value;
}

template <typename CharT, typename Iterator, typename Func>
std::basic_string<CharT>
process1(Iterator &begin, Iterator end, Func lookup, int delim)
{
    std::basic_string<CharT> result;
    CharT c;
    while (begin < end && (c = *begin++) != delim) {
        if (c == '$' && begin < end && *begin == '{')
            result += expand<CharT>(++begin, end, lookup);
        else
            result.push_back(c);
    }
    return result;
}

template <typename CharT, typename Func>
std::basic_string<CharT>
process_template(const std::basic_string<CharT> &s, Func lookup)
{
    typename std::basic_string<CharT>::const_iterator it = s.begin();
    return process1<CharT>(it, s.end(), lookup, 0);
}

#endif
