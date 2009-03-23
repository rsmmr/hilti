# $Id:$

############# Start of boiler-plate

%extern{
#include <bptool.h>
%}

analyzer Test withcontext {
    connection: Test_Conn;
    flow: Test_Flow;
};

type Test_Message(is_orig: bool) = case is_orig of {
    true ->  request: Test_Request;
    false -> reply:   Test_Reply;
};

############# End of boiler-plate

type Test_Request = record {
    foo:     uint8[] &until($element == 32);
    bar:     uint8[] &until($element == 10);
};

type Test_Reply = record {
    foo:     uint8[] &until($element == 32);
    bar:     uint8[] &until($element == 10);
};

%code{
void print_request(Test_Request* r)
{
    cerr << "request:" << endl;
    cerr << "  foo = " << r->foo() << endl;
    cerr << "  bar = " << r->bar() << endl;
}

void print_reply(Test_Reply* r)
{
    cerr << "reply:" << endl;
    cerr << "  foo = " << r->foo() << endl;
    cerr << "  bar = " << r->bar() << endl;
}
%}

############# Start of more boiler-plate

connection Test_Conn() {
    upflow = Test_Flow(true);
    downflow = Test_Flow(false);
};

flow Test_Flow(is_orig: bool) {
	datagram = Test_Message(is_orig) withcontext(connection, this);
    
    function print(req: bool) : bool 
    %{ 
        if ( req )
            print_request(dataunit_->request());
        else
            print_reply(dataunit_->reply());
    %}
};

refine typeattr Test_Request += &let {
        process: bool = $context.flow.print(true);
};

refine typeattr Test_Reply += &let {
        process: bool = $context.flow.print(false);
};

############# End of more boiler-plate


