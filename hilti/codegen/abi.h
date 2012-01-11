
#ifndef HILTI_CODEGEN_ABI_H
#define HILTI_CODEGEN_ABI_H

// This file is not yet used.

namespace llvm { class Value; }

namespace hilti {
namespace codegen {
namespace abi {

class Argument {
public:
   enum CType {
       VOID,
       SINT8, SINT16, SINT32, SINT64,
       UINT8, UINT16, UINT32, UINT64,
       SCHAR,
       UCHAR,
       FLOAT,
       DOUBLE,
       STRUCT,
       POINTER
   };

   enum Action {
       UNKNOWN,  /// Action not yet determined.
       DIRECT,   /// Pass value directly.
       INDIRECT, /// Pass as pointer to struct.
       EXPAND,   /// Expand aggregate into series of individual (direct) arguments.
   };

   Argument(CType type, llvm::Value* value) {
       _type = type;
       _value = value;
   }

   // For structs.
   Argument(CType type, llvm::Value* value, const argument_list& fields) {
       _type = type;
       _fields = fields;
   }

   CType type() const { return _type; }

   llvm::Value* value() const { return _value; }
   void setValue(llvm::Value* value) { _value = value; }

   // For structs.
   shared_ptr<Argument> fields() const { return _fields; }

   Action action() const  { return _action; }
   void setAction(Action action) const { _action = action; }

   int alignment() const { return _alignment; }
   void setAlignment(int alignment) { _alignment = alignment; }

private:
   CType _type;
   llvm::Value* _value;
   argument_list _fields;
   Action _action = UNKNOWN;
   int _alignment = 0;
};

extern const string& targetTriple();
extern bool isSupportedArchitecture();
extern bool isLittleEndian();
extern bool applyCallingConvention(shared_ptr<Argument> result, const std::list<shared_ptr<Argument>>& arguments);

}
}
}

#endif
