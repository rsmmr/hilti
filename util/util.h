// Misc utility functions.

#ifndef HILTI_UTIL_H
#define HILTI_UTIL_H

#include <list>
#include <string>

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

extern string strreplace(const string& s, const string& o, const string& n);
extern string strtolower(const string& s);
extern string strtoupper(const string& s);

inline bool startsWith(const string& s, const string& prefix) { return s.find(prefix) == 0; }
extern bool endsWith(const string& s, const string& suffix);

extern bool pathExists(const string& path);
extern bool pathIsFile(const string& path);
extern bool pathIsDir(const string& path);
extern string pathJoin(const string& p1, const string& p2);
extern string findInPaths(const string& file, const path_list& paths);
extern string dirname(const string& path);
extern string basename(const string& path);

}

#endif
