
#include "exceptions.h"
#include "libspicy.h"

hlt_exception_type spicy_exception_spicy = {"SpicyException", &hlt_exception_unspecified, 0};
hlt_exception_type spicy_exception_filtererror = {"FilterError", &spicy_exception_spicy,
                                                   &hlt_type_info_hlt_string};
hlt_exception_type spicy_exception_filterunsupported = {"FilterUnsupported",
                                                         &spicy_exception_spicy,
                                                         &hlt_type_info_hlt_string};
hlt_exception_type spicy_exception_notimplemented = {"NotImplemeneted", &spicy_exception_spicy,
                                                      &hlt_type_info_hlt_string};
hlt_exception_type spicy_exception_parserdisabled = {"ParserDisabled", &spicy_exception_spicy,
                                                      &hlt_type_info_hlt_string};
hlt_exception_type spicy_exception_valueerror = {"ValueError", &spicy_exception_spicy,
                                                  &hlt_type_info_hlt_string};
