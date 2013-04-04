
#include <iostream>
#include <fstream>
#include <utime.h>
#include <sys/stat.h>

#include "file-cache.h"
#include "util.h"

using namespace util;
using namespace util::cache;

bool FileCache::Key::operator==(const FileCache::Key& other) const
{
    if ( ! valid() )
        return false;

    if ( ! other.valid() )
        return false;

    if ( scope != other.scope )
        return false;

    if ( name != other.name )
        return false;

    if ( options != other.options )
        return false;

    if ( dirs.size() != other.dirs.size() )
        return false;

    for ( auto p : zip2(dirs, other.dirs) ) {
        if ( p.first != p.second )
            return false;
    }

    if ( files.size() != other.files.size() )
        return false;

    for ( auto p : zip2(files, other.files) ) {
        if ( p.first != p.second )
            return false;
    }

    if ( hashes.size() != other.hashes.size() )
        return false;

    for ( auto p : zip2(hashes, other.hashes) ) {
        if ( p.first != p.second )
            return false;
    }

    return true;
}

bool FileCache::Key::valid() const
{
    if ( _invalid )
        return false;

    if ( ! _timestamp )
        return true;

    for ( auto f : files ) {
        if ( modificationTime(f) > _timestamp )
            return false;
    }

    return true;
}

void FileCache::Key::setInvalid()
{
    _invalid = true;
}

bool FileCache::Key::operator!=(const FileCache::Key& other) const
{
    return ! (*this == other);
}

std::ostream& operator<<(std::ostream &out, const FileCache::Key& key)
{
    out << "v1" << std::endl;
    out << key.scope << std::endl;
    out << key.name << std::endl;
    out << key.options << std::endl;

    for ( auto d : key.dirs )
        out << "dir " << d << std::endl;

    for ( auto f : key.files )
        out << "file " << f << std::endl;

    for ( auto h : key.hashes )
        out << "hash " << h << std::endl;

    return out;
}

std::istream& operator>>(std::istream &in, FileCache::Key& key)
{
    string version;
    std::getline(in, version);

    if ( version != "v1" ) {
        key.setInvalid();
        return in;
    }

    std::getline(in, key.scope);
    std::getline(in, key.name);
    std::getline(in, key.options);

    while ( true ) {
        string line;
        std::getline(in, line);

        if ( ! in.good() )
            break;

        auto m = strsplit(line);
        auto i = m.begin();
        auto tag = *i++;
        auto val = *i++;

        if ( tag == "dir" )
            key.dirs.push_back(val);

        if ( tag == "file" )
            key.files.push_back(val);

        if ( tag == "hash" )
            key.hashes.push_back(val);
    }

    return in;
}


FileCache::FileCache(const string& dir)
{
    _dir = dir;

    if ( pathIsDir(dir) )
        return;

    if ( ! makeDir(dir) ) {
        std::cerr << "warning: cannot create cache directory " << dir << std::endl;
        _dir = "";
    }
}

FileCache::~FileCache()
{
}

bool FileCache::store(const Key& key, std::istream& data)
{
    if ( _dir.empty() )
        return false;

    auto fname = fileForKey(key);
    auto skey = string("key.") + fname;
    auto sdata = string("data.") + fname;

    auto out = std::ofstream(skey);
    out << key;
    out.close();

    out = std::ofstream(sdata);
    out << data.rdbuf();
    out.close();

    auto ct = currentTime();
    touchFile(skey, ct);
    touchFile(sdata, ct);

    return true;
}

bool FileCache::store(const Key& key, const string& data)
{
    if ( _dir.empty() )
        return false;

    std::stringstream s(data);
    return store(key, s);
}

bool FileCache::lookup(const Key& key, std::ostream& data)
{
    auto path = lookup(key);

    if ( path.empty() )
        return false;

    auto in = std::ifstream(path);

    if ( ! in.good() )
        return false;

    data << in.rdbuf();

    return true;
}

bool FileCache::lookup(const Key& key, string* data)
{
    if ( _dir.empty() )
        return false;

    std::stringstream s;
    if  ( ! lookup(key, s) )
        return false;

    *data = s.str();
    return true;
}

string FileCache::lookup(const Key& key)
{
    if ( _dir.empty() )
        return "";

    auto fname = fileForKey(key);

    auto in = std::ifstream(fname + ".key");

    if ( ! in.good() )
        return "";

    Key dkey;
    in >> dkey;

    dkey._timestamp = modificationTime(fname + ".key");

    if ( key != dkey )
        return "";

    auto path = fname + ".data";
    return pathIsFile(path) ? path : "";
}

string FileCache::fileForKey(const Key& key)
{
    return pathJoin(_dir, fmt("%s.%s.%s", key.name, key.options, key.scope));
}

bool FileCache::touchFile(const string& path, time_t time)
{
    struct utimbuf ut;
    ut.actime = time;
    ut.modtime = time;
    return utime(path.c_str(), &ut) == 0;
}

time_t cache::currentTime()
{
    return time(0);
}

time_t cache::modificationTime(const string& path)
{
    struct stat buf;

    if ( stat(path.c_str(), &buf) < 0 )
        return 0;

    return buf.st_mtime;
}

FileCache::Key::Hash cache::hash(std::istream& in)
{
    std::stringstream s;
    s << in.rdbuf();
    return uitoa_n(::util::hash(s.str()), 64);
}

FileCache::Key::Hash cache::hash(const string& path)
{
    if ( ! pathIsFile(path) )
        return "";

    std::stringstream s(path);
    return hash(s);
}
