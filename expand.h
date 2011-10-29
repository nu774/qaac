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
 *   otherwise literal word
 * - ${VAR|word} evaluates to ${VAR} if ${VAR} is not empty,
 *   otherwise literal word
 */

template <typename CharT, typename Func>
std::basic_string<CharT>
expand(const std::basic_string<CharT> &name, Func lookup)
{
    static CharT meta[] = { '&', '|', 0 };
    typename std::basic_string<CharT>::size_type
	pos = name.find_first_of(meta);
    if (pos == std::basic_string<CharT>::npos)
	return lookup(name);
    std::basic_string<CharT> value = lookup(name.substr(0, pos));
    if (name[pos] == '&')
	return value.empty() ? value : name.substr(pos + 1);
    else
	return value.empty() ? name.substr(pos + 1) : value;
}

template <typename CharT, typename Func>
std::basic_string<CharT>
process_template(const std::basic_string<CharT> &s, Func lookup)
{
    typedef std::char_traits<CharT> traits_type;
    enum State { INIT, DOLLAR, OPEN, NAME };

    std::basic_stringbuf<CharT> src(s);
    std::basic_string<CharT> acc, name;
    typename std::char_traits<CharT>::int_type c;
    State state = INIT;

    while (traits_type::not_eof(c = src.sbumpc())) {
	if (state == INIT) {
	    if (c == '$')
		state = DOLLAR;
	    else
		acc.push_back(c);
	} else if (state == DOLLAR) {
	    if (c == '{')
		state = OPEN;
	    else {
	    	state = INIT;
	    	acc.push_back('$');
	    	acc.push_back(c);
	    }
	} else if (c == '}') {
	    state = INIT;
	    acc += expand(name, lookup);
	    name.clear();
	} else
	    name.push_back(c);
    }
    if (state != INIT)
	acc.push_back('$');
    if (state == OPEN) {
	acc.push_back('{');
	acc += name;
    }
    return acc;
}

#endif
