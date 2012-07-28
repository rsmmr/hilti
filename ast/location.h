
#ifndef AST_LOCATION_H
#define AST_LOCATION_H

#include "common.h"

namespace ast {

/// Location information attached to AST nodes.
class Location {
public:

    /// Constructor. A location can have a filename and a range of line
    /// numbers.
    ///
    /// file: File name associated with the location. Empty if unknown.
    ///
    /// from: The first line number of the described range. -1 if not available.
    ///
    /// to: The last line number of the described range. -1 if not available.
    Location(const std::string& file = "", int from = -1, int to = -1) {
       _file = file; _from = from; _to = to;
    }

    Location(const Location&) = default;
    Location& operator=(const Location&) = default;
    ~Location() {}

    /// Returns true if the location is set. Being set is defined as not
    /// having been assigned, or constructed from, #None.
    operator bool() const { return _file != None._file; }

    /// Returns a readable representation of the location. It includes only
    /// information that's set.
    operator string() const;

    /// Sentinel value indicating that no location information is available.
    static const Location None;

private:
    string _file;
    int _from;
    int _to;
};

}

#endif
