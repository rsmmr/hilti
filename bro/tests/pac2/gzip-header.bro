
event gzip::member(f: fa_file, method: count, flags: count , mtime: time, xflags: count, os: gzip::OS)
{
    print f$id, method, flags, fmt("%.6f", mtime), xflags, os;
}
