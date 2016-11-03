#
# @TEST-EXEC: bro -r ${TRACES}/gzip-single-request.trace gzip.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#

type Flags: record  {
        ftext: count;
        fhrcr: count;
        fextra: count;
        fname: count;
        fcomment: count;
};

event gzip::member(f: fa_file, method: count, flags: Flags, mtime: time, xflags: count, os: gzip::OS)
{
    print f$id, method, flags, fmt("%.6f", mtime), xflags, os;
}

