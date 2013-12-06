#
# @TEST-EXEC: bro -r ${TRACES}/ssh-single-conn.trace ./ssh-tuple.evt %INPUT >output
# @TEST-EXEC: btest-diff output

type Foo: record {
	i: int;
	s: string;
};

event ssh::banner(f: Foo)
	{
	print f;
	}

# @TEST-START-FILE ssh-tuple.evt

grammar ssh.pac2;

protocol analyzer pac2::SSH over TCP:
    parse with SSH::Banner,
    port 22/tcp,
    replaces SSH;

on SSH::Banner -> event ssh::banner( (1, self.software) );

# @TEST-END-FILE
