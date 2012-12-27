
#include <algorithm>

#include <sys/stat.h>
#include <execinfo.h>

#include "util.h"

namespace utf8 {
// We include these here directly, and withan a namespace, so that the
// function names don't clash with those in libhilti-rt.
#include "3rdparty/utf8proc/utf8proc.h"
#include "3rdparty/utf8proc/utf8proc.c"
}

using namespace util;

#if 0
string util::fmt(const char* fmt, ...)
{
    char buffer[1024];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    buffer[sizeof(buffer)-1] = '\0';
	va_end(ap);

    return string(buffer);
}
#endif

std::list<string> util::strsplit(string s, string delim)
{
    std::list<string> l;

    while ( true ) {
        size_t p = s.find(delim);

        if ( p == string::npos )
            break;

        l.push_back(s.substr(0, p));

        // FIXME: Don't understand why directly assigning to s doesn't work.
        string t = s.substr(p + delim.size(), string::npos);
        s = t;
    }

    l.push_back(s);
    return l;
}

string util::strreplace(const string& s, const string& o, const string& n)
{
    string r = s;

    while ( true ) {
        auto i = r.find(o);

        if ( i == std::string::npos )
            break;

        r.replace(i, o.size(), n);
    }

    return r;
}

string util::strtolower(const string& s)
{
    string t = s;
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    return t;
}

string util::strtoupper(const string& s)
{
    string t = s;
    std::transform(t.begin(), t.end(), t.begin(), ::toupper);
    return t;
}


string util::strtrim(const string& s)
{
    auto t = s;
    t.erase(t.begin(), std::find_if(t.begin(), t.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    t.erase(std::find_if(t.rbegin(), t.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), t.end());
    return t;
}

uint64_t util::hash(const string& str)
{
    uint64_t h = 0;

    for ( int i = 0; i < str.size(); i++ )
        h = (h << 5) - h + (uint64_t)str[i];

	return h;
}

string util::uitoa_n(uint64_t value, int base, int n)
{
    static char dig[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    string s;

	do {
		s.append(1, dig[value % base]);
		value /= base;
	} while ( value && (n < 0 || s.size() < n - 1 ));

	return s;
}

bool util::pathExists(const string& path)
{
    struct stat st;
    return ::stat(path.c_str(), &st) == 0;
}

bool util::pathIsFile(const string& path)
{
    struct stat st;
    if ( ::stat(path.c_str(), &st) < 0 )
        return false;

    return S_ISREG(st.st_mode);
}

bool util::pathIsDir(const string& path)
{
    struct stat st;
    if ( ::stat(path.c_str(), &st) < 0 )
        return false;

    return S_ISDIR(st.st_mode);
}

bool util::endsWith(const string& s, const string& suffix)
{
    size_t i = s.rfind(suffix);

    if ( i == string::npos )
        return false;

    return (i == (s.length() - suffix.size()));
}

string util::expandEscapes(const string& s)
{
    string d;

    for ( auto c = s.begin(); c != s.end(); ) {

        if ( *c != '\\' ) {
            d += *c++;
            continue;
        }

        ++c;

        if ( c == s.end() )
            throw std::runtime_error("broken escape sequence");

        switch ( *c++ ) {
         case '\\':
            d += '\\';
            break;

         case '"':
            d += '"';
            break;

         case 'n':
            d += '\n';
            break;

         case 'r':
            d += '\r';
            break;

         case 't':
            d += '\t';
            break;

         case 'u': {
             int32_t val;
             auto end = c + 4;
             c = atoi_n(c, end, 16, &val);

             if ( c != end )
                 throw std::runtime_error("cannot decode character");

             uint8_t tmp[4];
             int len = utf8::utf8proc_encode_char(val, tmp);

             if ( ! len )
                 throw std::runtime_error("cannot encode unicode code point");

             d.append((char *)tmp, len);
             break;
         }

         case 'U': {
             int32_t val;
             auto end = c + 8;
             c = atoi_n(c, end, 16, &val);

             if ( c != end )
                 throw std::runtime_error("cannot decode character");

             uint8_t tmp[4];
             int len = utf8::utf8proc_encode_char(val, tmp);

             if ( ! len )
                 throw std::runtime_error("cannot encode unicode code point");

             d.append((char *)tmp, len);
             break;
         }

         case 'x': {
             char val;
             auto end = c + 2;
             c = atoi_n(c, end, 16, &val);

             if ( c != end )
                 throw std::runtime_error("cannot decode character");

             d.append(&val, 1);
             break;
         }

         default:
            throw std::runtime_error("unknown escape sequence");

        }
    }

    return d;
}

string util::escapeUTF8(const string& s)
{
    const unsigned char* p = (const unsigned char*)s.data();
    const unsigned char* e = p + s.size();

    string esc;

    while ( p < e ) {
        int32_t cp;

        ssize_t n = utf8::utf8proc_iterate((const uint8_t *)p, e - p, &cp);

        if ( n < 0 ) {
            esc += "<illegal UTF8 sequence>";
            break;
        }

        if ( cp == '\\' )
            esc += "\\\\";

        else if ( cp == '"' )
            esc += "\\\"";

        else if ( cp == '\n' )
            esc += "\\n";

        else if ( cp == '\r' )
            esc += "\\t";

        else if ( cp == '\t' )
            esc += "\\t";

        else if ( cp >= 65536 )
            esc += fmt("\\U%08x", cp);

        else if ( ! isprint((char)cp) )
            esc += fmt("\\u%04x", cp);

        else
            esc += (char)cp;

        p += n;
    }

    return esc;
}

string util::escapeBytes(const string& s)
{
    const unsigned char* p = (const unsigned char*)s.data();
    const unsigned char* e = p + s.size();

    string esc;

    while ( p < e ) {
        if ( *p == '\\' )
            esc += "\\\\";

        else if ( *p == '"' )
            esc += "\\\"";

        else if ( *p == '\n' )
            esc += "\\n";

        else if ( *p == '\r' )
            esc += "\\t";

        else if ( *p == '\t' )
            esc += "\\t";

        else if ( isprint(*p) )
            esc += *p;

        else
            esc += fmt("\\x%02x", *p);

        ++p;
    }

    return esc;
}

string util::pathJoin(const string& p1, const string& p2)
{
    if ( startsWith(p2, "/") )
        return p2;

    string p = p1;

    while ( endsWith(p, "/") )
        p = p.substr(0, p.size() - 1);

    return p + "/" + p2;
}

string util::dirname(const string& path)
{
    size_t i = path.rfind("/");

    if ( i == string::npos )
        return "";

    string dir = path.substr(0, i);

    string p = path;

    while ( endsWith(p, "/") )
        p = p.substr(0, p.size() - 1);

    return p;
}

string util::basename(const string& path)
{
    size_t i = path.rfind("/");

    if ( i == string::npos )
        return path;

    return path.substr(i+1, string::npos);
}


string util::findInPaths(const string& file, const path_list& paths)
{
    for ( auto d : paths ) {
        string path = pathJoin(d, file);
        if ( pathIsFile(path) )
            return path;
    }

    return "";
}

void util::abort_with_backtrace()
{
    fputs("\n--- Aborting\n", stderr);
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    backtrace_symbols_fd(callstack, frames, 2);
    abort();
}

