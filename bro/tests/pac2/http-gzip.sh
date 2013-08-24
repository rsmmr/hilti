#
# @TEST-EXEC: ${EVAL}/check-http ${TRACES}/http/get-gzip.trace
# @TEST-EXEC: btest-diff http-events-pac.log
# @TEST-EXEC: btest-diff results.events.txt
#
