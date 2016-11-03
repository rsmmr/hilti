#
# @TEST-EXEC: ${EVAL}/check-http ${TRACES}/http/get-gzip.trace
# @TEST-EXEC: TEST_DIFF_CANONIFIER=${SCRIPTS}/bro-events-canonifier btest-diff http-events-spicy.log
# @TEST-EXEC: TEST_DIFF_CANONIFIER=${SCRIPTS}/bro-events-canonifier btest-diff results.events.txt
#
