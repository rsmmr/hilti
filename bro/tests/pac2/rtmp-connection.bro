#
# @TEST-EXEC: bro -r ${TRACES}/rtmp.trace rtmp.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#

event rtmp::object_string(c: connection, key: string, value: string)
	{
	print key, value;
	}
