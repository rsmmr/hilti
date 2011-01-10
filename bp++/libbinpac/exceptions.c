
#include "binpac.h"

extern hlt_type_info hlt_type_info_string;

hlt_exception_type binpac_exception_binpac = { "BinPACException", &hlt_exception_unspecified, 0 };
hlt_exception_type binpac_exception_filtererror = { "FilterError", &binpac_exception_binpac, &hlt_type_info_string };
hlt_exception_type binpac_exception_filterunsupported = { "FilterUnsupported", &binpac_exception_binpac, &hlt_type_info_string };
