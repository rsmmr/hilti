
// When adding new runtime functions, you must extend this function table or
// the JIT compiler will not be able to find them.

namespace bro {
namespace hilti {

extern const ::hilti::CompilerContext::FunctionMapping libbro_function_table[] = {
	{ "libbro_cookie_to_conn_val", (void*)&libbro_cookie_to_conn_val },
	{ "libbro_cookie_to_file_val", (void*)&libbro_cookie_to_file_val },
	{ "libbro_cookie_to_is_orig", (void*)&libbro_cookie_to_is_orig },
	{ "libbro_h2b_integer_signed", (void*)&libbro_h2b_integer_signed},
	{ "libbro_h2b_integer_unsigned", (void*)&libbro_h2b_integer_unsigned},
	{ "libbro_h2b_address", (void*)&libbro_h2b_address},
	{ "libbro_h2b_double", (void*)&libbro_h2b_double},
	{ "libbro_h2b_string", (void*)&libbro_h2b_string},
	{ "libbro_h2b_time", (void*)&libbro_h2b_time},
	{ "libbro_h2b_enum", (void*)&libbro_h2b_enum},
	{ "libbro_h2b_bytes", (void*)&libbro_h2b_bytes},

	{ "libbro_b2h_string", (void*)&libbro_b2h_string},

	{ "libbro_get_event_handler", (void*)&libbro_get_event_handler },
	{ "libbro_raise_event", (void*)&libbro_raise_event },
	{ "libbro_call_bif_void", (void*)&libbro_call_bif_void },
	{ "libbro_call_bif_result", (void*)&libbro_call_bif_result },

	{ "bro_file_begin", (void*)bro_file_begin },
	{ "bro_file_set_size", (void*)bro_file_set_size },
	{ "bro_file_data_in", (void*)bro_file_data_in },
	{ "bro_file_gap", (void*)bro_file_gap },
	{ "bro_file_end", (void*)bro_file_end },
	{ "bro_rule_match", (void*)bro_rule_match },

	{ "hilti_is_compiled", (void*)bif_hilti_is_compiled },
	{ "hilti_test_type_string", (void*)bif_hilti_test_type_string },
	{ 0, 0 } // End marker.
};

}
}
