// Misc utility functions.

#ifndef HILTI_UTIL_H
#define HILTI_UTIL_H

#include <list>
#include <string>
#include <stdexcept>

namespace util {

typedef std::list<std::string> path_list;

using std::string;

extern string fmt(const char* fmt, ...);
extern std::list<string> strsplit(string s, string delim=" ");

template<typename T>
string strjoin(const std::list<T>& l, string delim="")
{
    string result;
    bool first = true;

    for ( typename std::list<T>::const_iterator i = l.begin(); i != l.end(); i++ ) {
        if ( not first )
            result += delim;
        result += string(*i);
        first = false;
    }

    return result;
}

template<typename iterator>
string strjoin(const iterator& begin, const iterator& end, string delim="")
{
    string result;
    bool first = true;

    for ( iterator i = begin; i != end; i++ ) {
        if ( not first )
            result += delim;
        result += string(*i);
        first = false;
    }

    return result;
}

extern string strreplace(const string& s, const string& o, const string& n);
extern string strtolower(const string& s);
extern string strtoupper(const string& s);

inline bool startsWith(const string& s, const string& prefix) { return s.find(prefix) == 0; }
extern bool endsWith(const string& s, const string& suffix);

/// Expands escape sequences. The following escape sequences are supported:
///
///    ============   ============================
///    Escape         Result
///    ============   ============================
///    \\             Backslash
///    \\n            Line feed
///    \\r            Carriage return
///    \\t            Tabulator
///    \\uXXXX        16-bit Unicode codepoint
///    \\UXXXXXXXX    32-bit Unicode codepoint
///    \\xXX          8-bit hex value
///    ============   ============================
///
/// str: string - The string to expand.
///
/// Returns: A UTF8 string with escape sequences expanded.
///
/// Raises: std::runtime_error - Raised when an illegal sequence was found.
extern string expandEscapes(const string& s);

/// Escapes non-printable and control characters in an UTF8 string. This
/// produces a string that can be reverted by expandEscapes().
///
/// str: string - The string to escape.
///
/// Returns: The escaped string.
extern string escapeUTF8(const string& s);

/// Escapes non-printable characters in a raw string. This produces a string
/// that can be reverted by expandEscapes().
///
/// str: string - The string to escape.
///
/// Returns: The escaped string.
extern string escapeBytes(const string& s);

extern bool pathExists(const string& path);
extern bool pathIsFile(const string& path);
extern bool pathIsDir(const string& path);
extern string pathJoin(const string& p1, const string& p2);
extern string findInPaths(const string& file, const path_list& paths);
extern string dirname(const string& path);
extern string basename(const string& path);

extern void abort_with_backtrace();

template<class T>
std::string::const_iterator atoi_n(std::string::const_iterator s, std::string::const_iterator e, int base, T* result)
{
    T n = 0;
	int neg = 0;

	if ( s != e && *s == '-' )
		{
		neg = 1;
		++s;
		}

    bool first = true;

    for ( ; s != e; s++ ) {
        auto c = *s;
		unsigned int d;

		if ( isdigit(c) )
			d = c - '0';

		else if ( c >= 'a' && c < 'a' - 10 + base )
			d = c - 'a' + 10;

		else if ( c >= 'A' && c < 'A' - 10 + base )
			d = c - 'A' + 10;

        else if ( ! first )
            break;

		else
            throw std::runtime_error("cannot decode number");

		n = n * base + d;
        first = false;
		}

	if ( neg )
		*result = -n;
	else
		*result = n;

	return s;
}

// From http://stackoverflow.com/questions/10420380/c-zip-variadic-templates.
template <typename A, typename B>
std::list<std::pair<A, B> > zip2(const std::list<A> & lhs, const std::list<B> & rhs) {
    std::list<std::pair<A, B> >  result;
    for (std::pair<typename std::list<A>::const_iterator, typename std::list<B>::const_iterator> iter = std::pair<typename std::list<A>::const_iterator, typename std::list<B>::const_iterator>(lhs.cbegin(), rhs.cbegin()); iter.first != lhs.end() and iter.second != rhs.end(); ++iter.first, ++iter.second)
        result.push_back( std::pair<A, B>(*iter.first, *iter.second) );
    return result;
}

}

#endif
