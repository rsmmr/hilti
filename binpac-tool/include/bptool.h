
#ifndef BPTOOL_H
#define BPTOOL_H

#include <iostream>
#include <iomanip>

extern int bptool_verbose;

inline
std::ostream& operator << (std::ostream& os, vector<binpac::uint8>* v)
{
    for ( vector<binpac::uint8>::iterator i = v->begin(); i != v->end(); i++ )
        {
        char c = *i;
        if ( isprint(c) )
            os << c;
        else
            os << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int) c;
        }

    return os;
}

#endif
