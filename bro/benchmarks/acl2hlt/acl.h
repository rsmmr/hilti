/* Automatically generated. Do not edit. */

#ifndef ACL2HLT_HLT_H
#define ACL2HLT_HLT_H

#include <libhilti/libhilti.h>

void acl2hlt_init_classifier(hlt_exception** excpt, hlt_execution_context* ctx);
void acl2hlt_init_classifier_resume(hlt_exception* yield_excpt, hlt_exception** excpt, hlt_execution_context* ctx);
int8_t acl2hlt_match_packet(hlt_time t, hlt_addr src, hlt_addr dst, hlt_exception** excpt, hlt_execution_context* ctx);
int8_t acl2hlt_match_packet_resume(hlt_exception* yield_excpt, hlt_exception** excpt, hlt_execution_context* ctx);
#endif
