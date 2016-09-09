#
# @TEST-EXEC: ${EVAL}/check-http ${TRACES}/http/pipelined-requests.trace
# @TEST-EXEC: TEST_DIFF_CANONIFIER=${SCRIPTS}/bro-events-canonifier btest-diff http-events-pac.log
# @TEST-EXEC: TEST_DIFF_CANONIFIER=${SCRIPTS}/bro-events-canonifier btest-diff results.events.txt
#
