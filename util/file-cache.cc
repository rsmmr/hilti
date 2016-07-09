
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <utime.h>

#include "file-cache.h"
#include "util.h"

using namespace util;
using namespace util::cache;

bool FileCache::Key::operator==(const FileCache::Key& other) const
{
#if 0
    std::cerr << "%%%%%%" << std::endl;
    std::cerr << *this << std::endl;
    std::cerr << other << std::endl;
    std::cerr << "%%%%%%" << std::endl;
#endif

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

    if ( ! util::set_difference(dirs, other.dirs).empty() )
        return false;

    if ( files.size() != other.files.size() )
        return false;

    if ( ! util::set_difference(files, other.files).empty() )
        return false;

    if ( hashes.size() != other.hashes.size() )
        return false;

    if ( ! util::set_difference(hashes, other.hashes).empty() )
        return false;

#if 0
    std::cerr << "--->>>> EQUAL" << std::endl;
#endif

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

std::ostream& operator<<(std::ostream& out, const FileCache::Key& key)
{
    out << "v1" << std::endl;
    out << key.scope << std::endl;
    out << key.name << std::endl;
    out << key.options << std::endl;
    out << key._parts << std::endl;

    for ( auto d : key.dirs )
        out << "dir " << d << std::endl;

    for ( auto f : key.files )
        out << "file " << f << std::endl;

    for ( auto h : key.hashes )
        out << "hash " << h << std::endl;

    return out;
}

std::istream& operator>>(std::istream& in, FileCache::Key& key)
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

    string parts;
    std::getline(in, parts);
    key._parts = std::atoi(parts.c_str());

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
            key.dirs.insert(val);

        if ( tag == "file" )
            key.files.insert(val);

        if ( tag == "hash" )
            key.hashes.insert(val);
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

bool FileCache::store(const Key& key, const char* data, size_t len)
{
    std::list<std::pair<const char*, size_t>> l;
    l.push_back(std::make_pair(data, len));
    return store(key, l);
}

bool FileCache::store(const Key& key, std::list<std::pair<const char*, size_t>> data)
{
    if ( _dir.empty() )
        return false;

    std::list<string> l;

    for ( auto d : data )
        l.push_back(string(d.first, d.second));

    return store(key, l);
}

bool FileCache::store(const Key& key, std::list<std::string> data)
{
    if ( _dir.empty() )
        return false;

    auto ct = currentTime();
    auto skey = fileForKey(key, "key");
    auto out = std::ofstream(skey);

    Key ckey(key);
    ckey._parts = data.size();

    out << ckey;
    out.close();

    touchFile(skey, ct);

    int idx = 1;

    for ( auto s : data ) {
        auto sdata = fileForKey(key, "data", idx++);

        out = std::ofstream(sdata);
        out.write(s.data(), s.size());
        out.close();

        touchFile(sdata, ct);
    }

    return true;
}

std::list<string> FileCache::lookup(const Key& key)
{
    std::list<string> outputs;

    if ( _dir.empty() )
        return outputs;

    auto keyfile = fileForKey(key, "key");
    auto in = std::ifstream(keyfile);

    if ( ! in.good() )
        return outputs;

    Key dkey;
    in >> dkey;

    dkey._timestamp = modificationTime(keyfile);

    if ( key != dkey )
        return outputs;

    for ( int idx = 1; idx <= dkey._parts; idx++ ) {
        auto path = fileForKey(key, "data", idx);

        if ( pathIsFile(path) ) {
            std::ifstream f(path);
            outputs.push_back(
                std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()));
        }
    }

    return outputs;
}

string FileCache::fileForKey(const Key& key, const string& prefix, int idx)
{
    string part = idx > 0 ? ::util::fmt(".%d", idx) : string();
    return pathJoin(_dir, fmt("%s.%s.%s%s.%s", prefix, key.name, key.options, part, key.scope));
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

FileCache::Key::Hash cache::hash(const string& s)
{
    return uitoa_n(::util::hash(s.data(), s.size()), 64);
}

FileCache::Key::Hash cache::hash(const char* data, size_t len)
{
    return uitoa_n(::util::hash(data, len), 64);
}

FileCache::Key::Hash cache::hash(size_t size)
{
    return uitoa_n(size, 64);
}
