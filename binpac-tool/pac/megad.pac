# Direction can be <inbound|outbound>,
#  <outbound> means BOT -> CC
#  <inbound> means CC -> BOT

%extern{
#include <bptool.h>

#define ntohll(x) (((uint64_t)(ntohl((uint32_t)(x & 0xffffffffULL))) << 32) | \
  		  (((uint64_t)(ntohl((uint32_t)(x >> 32)))) & 0xffffffffULL))

#define htonll(x) ntohll(x)
extern "C" { 
extern uint8_t decode_key[];
void megadfun( uint8_t *key, uint64_t *src, uint64_t *src2 );
}

%}

analyzer MegaD withcontext {
    connection: MegaD_Conn;
    flow: MegaD_Flow;
};

type MegaD_Message(is_orig: bool) = record {
  len : uint16; # MSG_LEN = msg_len * 8 + 2;
  encrypted_payload: bytestring &restofdata;
} &byteorder = bigendian
  &let { 
    actual_length: uint16 = len * 8; # + 2 -2 for the uint16 of len
    payload : msg_payload withinput($context.flow.decrypt(encrypted_payload));
};

type msg_payload = record {
  version : uint16; # Constant(0x100)
  mtype : uint16;
  data : MegaD_data(mtype);
};

type MegaD_data(msg_type: uint16) = case msg_type of {
  0x00 -> m00: msg_0;
  0x01 -> m01: msg_1;
  0x15 -> m15: msg_0x15;
  0x16 -> m16: msg_0x16;
  0x18 -> m18: msg_0x18;
  0x21 -> m21: msg_0x21;
  0x22 -> m22: msg_0x22;
  0x23 -> m23: msg_0x23;
  0x24 -> m24: msg_0x24;
  0x25 -> m25: msg_0x25;
  
  # There are several other message types, will add later
  default -> unknown : bytestring &restofdata;
};

# Direction: outbound
# Interpretation: initial message to signal host is owned, but used later in dialog too
type msg_0 = record {
  fld_00 : uint8;  # Constant(0)
  fld_01 : bytestring &length = 16;  # This field is the ID received from the CC in msg_1, but in the initial message it is always zero
  fld_02 : uint32; # Constant (0xd)
  fld_03 : uint32; # Constant (0x26)
  fld_04 : uint32; # IP address of bot returned by ws2_32.dll::getsockname
};

# Direction: inbound
# Interpretation: ??
type msg_1 = record {
  fld_00 : bytestring &length = 16; # This value is stored in the registry in key: "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Fci\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  fld_01 : uint32;  # Number of seconds to sleep after reception of this message. Multiplied by 1000 and used as parameter to Sleep function (takes as input milliseconds)
  fld_02_a : uint32;  # Bot ID, part 1 (BinPAC doesn't have uint64)
  fld_02_b : uint32;  # Bot ID, part 2
};

# Direction: inbound
# Interpretation: STMP MegaD info
type msg_0x21 = record {
  fld_00 : uint32; # ?? Divides by 16 and checks if it is Constant(1)
  fld_01 : uint16; # Port to connect to (Used as parameter to connect function)
  fld_02 : uint8[] &until($element == 0); # hostname for SMTP server to MegaD SPAM sending capabilities
  pad : bytestring &restofdata;
};

# Direction: outbound
# Interpretation: Spam MegaD failed
type msg_0x23 = record {
  fld_00 : uint32;  # Error code. Constant(0x4), possibly OR'ed with value returned by ntdll.dll::RtlGetLastWin32Error
  fld_01_a : uint32;  # Bot ID, part 1 (BinPAC doesn't have uint64)
  fld_01_b : uint32;  # Bot ID, part 2
};

# Direction: outbound
# Interpretation: Spam MegaD succeded
type msg_0x22 = record {
  fld_00_a : uint32;  # Bot ID, part 1 (BinPAC doesn't have uint64)
  fld_00_b : uint32;  # Bot ID, part 2
  pad : bytestring &restofdata;
};

# Direction: inbound
# Interpretation: Connect to given address+port to download template/addresses
type msg_0x24 = record {
  fld_00 : uint32; # IP address to connect to
  fld_01 : uint16; # Port to connect to
  pad : bytestring &restofdata;
};

# Direction: outbound
# Interpretation: Confirmation that address to download template/addresses has been obtained
type msg_0x25 = record {
  fld_00_a : uint32;  # Bot ID, part 1 (BinPAC doesn't have uint64)
  fld_00_b : uint32;  # Bot ID, part 2
  pad : bytestring &restofdata;
};

# Direction: inbound
# Interpretation: ??
type msg_0x18 = record {
  fld_00_a : uint32;  # Bot ID, part 1 (BinPAC doesn't have uint64)
  fld_00_b : uint32;  # Bot ID, part 2
  pad : bytestring &restofdata;
};

# Direction: inbound
# Interpretation: Request for host information
type msg_0x15 = record {
  pad : bytestring &restofdata;
};

type host_info = record {
  fld_00 : uint32; # Value returned by cpuid instruction (It is always "Genu" from the string "GenuineIntel", returned by cpuid in EBX)
  fld_01 : uint32; # Value computed from output of rdtsc instruction (rdtsc -> a; Sleep(1000); rdtsc -> b; b - a -> c; c / $0xf4240 -> MSG)
  fld_02 : uint32; # Quotient of dividing the output of kernel32.dll::GetTickCount by 1000
  fld_03 : uint16; # dwMajorVersion output by kernel32.dll::GetVersionExA
  fld_04 : uint16; # dwMinorVersion output of kernel32.dll::GetVersionExA
  fld_05 : uint16; # dwBuildNumber output of kernel32.dll::GetVersionExA
  fld_06 : uint16; # wServicePackMajor output of kernel32.dll::GetVersionExA
  fld_07 : uint16; # wServicePackMinor output of kernel32.dll::GetVersionExA
  fld_08 : uint32; # dwTotalPhys (amount of physical memory) output by kernel32.dll::GlobalMemoryStatus divided by 2^10 (thus, in KB)
  fld_09 : uint32; # dwAvailPhys (amount of available memory) output by kernel32.dll::GlobalMemoryStatus divided by 2^10 (thus, in KB)
  fld_10 : uint16; # lpdwFlags (type of connection to the Internet) output by wininet.dll::InternetConnectedStateEx
  fld_11 : uint32; # IP address output by ws2_32.dll::getsockname
};

# Direction: outbound
# Interpretation: Reply with host information
type msg_0x16 = record {
  fld_00_a : uint32;  # Bot ID, part 1 (BinPAC doesn't have uint64)
  fld_00_b : uint32;  # Bot ID, part 2
  fld_01 : uint16; # Length of host_info (each store uses stos %eax,%es:(%edi) which increases edi, then at end substract starting pointer from %edi)
  fld_02 : host_info;
  pad : bytestring &restofdata;
};

connection MegaD_Conn() {
    upflow = MegaD_Flow(true);
    downflow = MegaD_Flow(false);
};

flow MegaD_Flow(is_orig: bool) {
	datagram = MegaD_Message(is_orig) withcontext(connection, this);

    function check_len(msg: MegaD_Message) : bool
    %{     
        return msg->encrypted_payload().length() == msg->actual_length();
    %}
    
    function print(msg: MegaD_Message) : bool 
    %{     
        cerr << "message:" << endl;
        cerr << "  len = " << msg->len() << endl;
        cerr << "  actual length = " << msg->actual_length() << endl;
        cerr << "  check_len = " << msg->check_len() << endl;
        cerr << "  version = " << msg->payload()->version() << endl;
        cerr << "  type = " << msg->payload()->mtype() << endl;
    %}
    
    function decrypt(data: const_bytestring) : const_bytestring
    %{     
        
        if ( data.length() % 8 != 0 )
            cerr << "message length not a multiple of eight; can't decrypt" << endl;

        static uint8 decrypted[1024]; // FIXME
                
        const binpac::uint8* i = data.begin();
        int j = 0;
        
        while ( i != data.end() ) {
            uint64_t src = 0;
            src |= ((uint64_t)*i++) << 0;
            src |= ((uint64_t)*i++) << 8;
            src |= ((uint64_t)*i++) << 16;
            src |= ((uint64_t)*i++) << 24;
            src |= ((uint64_t)*i++) << 32;
            src |= ((uint64_t)*i++) << 40;
            src |= ((uint64_t)*i++) << 48;
            src |= ((uint64_t)*i++) << 56;
            megadfun(decode_key, &src, &src);
            *((uint64_t*)(decrypted + j)) = src;
            j += 8;
        }

        return const_bytestring(decrypted, decrypted + sizeof(decrypted));
    %}
    
};

refine typeattr MegaD_Message += &let {
    check_len: bool = $context.flow.check_len(this);
    print: bool = $context.flow.print(this);
};



