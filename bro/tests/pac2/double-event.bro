#
# @TEST-EXEC: bro -r ${TRACES}/ssh-single-conn.trace ./ssh-cond.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#

event ssh::banner(i: int, software: string)
	{
	print i, software;
	}

# @TEST-START-FILE ssh-cond.evt

grammar ssh.pac2;

protocol analyzer pac2::SSH over TCP:
    parse with SSH::Banner,
    port 22/tcp,
    replaces SSH;

on SSH::Banner -> event ssh::banner(1, self.software);
on SSH::Banner -> event ssh::banner(2, self.software);

# @TEST-END-FILE
