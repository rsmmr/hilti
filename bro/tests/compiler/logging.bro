#
# @TEST-EXEC: bro -b %INPUT
# @TEST-EXEC: TEST_DIFF_CANONIFIER=$SCRIPTS/diff-remove-timestamps btest-diff ssh.log
# @TEST-EXEC: TEST_DIFF_CANONIFIER=$SCRIPTS/diff-remove-timestamps btest-diff test.success.log
# @TEST-EXEC: TEST_DIFF_CANONIFIER=$SCRIPTS/diff-remove-timestamps btest-diff test.failure.log

module SSH;

export {
	redef enum Log::ID += { LOG };

	type Log: record {
		id: conn_id; # Will be rolled out into individual columns.
		status: string &optional;
		country: string &default="unknown";
	} &log;
}

global ssh_log: event(rec: Log);

function fail(rec: Log): bool
	{
	return rec$status != "success";
	}

event bro_init() &priority=-1
{
	print "1";
	Log::create_stream(SSH::LOG, [$columns=Log, $ev=ssh_log]);

	Log::add_filter(SSH::LOG, [$name="f1", $path="test.success", $pred=function(rec: Log): bool { return rec$status == "success"; }]);
	Log::add_filter(SSH::LOG, [$name="f2", $path="test.failure", $pred=fail]);

	local cid = [$orig_h=1.2.3.4, $orig_p=1234/tcp, $resp_h=2.3.4.5, $resp_p=80/tcp];
	Log::write(SSH::LOG, [$id=cid, $status="success"]);
	Log::write(SSH::LOG, [$id=cid, $status="failure", $country="US"]);
	Log::write(SSH::LOG, [$id=cid, $status="failure", $country="UK"]);
	Log::write(SSH::LOG, [$id=cid, $status="success", $country="BR"]);
	Log::write(SSH::LOG, [$id=cid, $status="failure", $country="MX"]);

	print "2";
}

event ssh_log(rec: Log)
	{
	print "ev", rec;
	}
