
#ifndef HILTI_CODEGEN_PROTOGEN_H
#define HILTI_CODEGEN_PROTOGEN_H

#include <fstream>

#include "../common.h"
#include "../visitor.h"

#include "common.h"

namespace hilti {
namespace codegen {

namespace protogen {

/// Visitor that maps HILTI types to their corresponding C types.
class TypeMapper : public hilti::Visitor<string>
{
public:
   TypeMapper() {}
   virtual ~TypeMapper();

   // Returns a string with the C type corresponding to a given HILTI type.
   string mapType(shared_ptr<Type> type);

protected:
   void visit(type::Address* t) override { setResult("hlt_addr"); }
   void visit(type::Any* t) override { setResult("void*"); }
   void visit(type::Bitset* t) override { setResult("hlt_bitset"); }
   void visit(type::Bool* t) override { setResult("int8_t"); }
   void visit(type::Bytes* t) override { setResult("hlt_bytes*"); }
   void visit(type::CAddr* t) override { setResult("void*"); }
   void visit(type::Callable* t) override { setResult("hlt_callable*"); }
   void visit(type::Channel* t) override { setResult("hlt_channel*"); }
   void visit(type::Classifier* t) override { setResult("hlt_classifier*"); }
   void visit(type::Double* t) override { setResult("double"); }
   void visit(type::Enum* t) override { setResult("hlt_enum"); }
   void visit(type::Exception* t) override { setResult("hlt_exception*"); }
   void visit(type::File* t) override { setResult("hlt_file*"); }
   void visit(type::IOSource* t) override { setResult("hlt_iosrc*"); }
   void visit(type::Integer* t) override;
   void visit(type::Interval* t) override { setResult("hlt_interval"); }
   void visit(type::List* t) override { setResult("hlt_list*"); }
   void visit(type::Map* t) override { setResult("hlt_map*"); }
   void visit(type::MatchTokenState* t) override { setResult("hlt_match_token_state"); }
   void visit(type::Network* t) override { setResult("hlt_network"); }
   void visit(type::Overlay* t) override { setResult("hlt_overlay*"); }
   void visit(type::Port* t) override { setResult("hlt_port"); }
   void visit(type::Reference* t) override;
   void visit(type::RegExp* t) override { setResult("hlt_regexp*"); }
   void visit(type::Set* t) override { setResult("hlt_set*"); }
   void visit(type::String* t) override { setResult("hlt_string"); }
   void visit(type::Struct* t) override;
   void visit(type::Time* t) override { setResult("hlt_time"); }
   void visit(type::Timer* t) override { setResult("hlt_timer*"); }
   void visit(type::TimerMgr* t) override { setResult("hlt_timer_mgr*"); }
   void visit(type::Tuple* t) override;
   void visit(type::Vector* t) override { setResult("hlt_vector*"); }
   void visit(type::Void* t) override { setResult("void"); }

   void visit(type::iterator::Bytes* i) override { setResult("hlt_iterator_bytes"); }
   void visit(type::iterator::List* i) override { setResult("hlt_iterator_list"); }
   void visit(type::iterator::Map* i) override { setResult("hlt_iterator_map"); }
   void visit(type::iterator::Set* i) override { setResult("hlt_iterator_set"); }
   void visit(type::iterator::Vector* i) override { setResult("hlt_iterator_vector"); }
};

}

/// Visitor that generates C prototypes to interface to the generated code.
class ProtoGen : public hilti::Visitor<>
{
public:
   /// Constructor.
   ///
   /// out: The output stream.
   ProtoGen(std::ostream& out) : _output(out) { }
   virtual ~ProtoGen() {};

   /// Writes prototypes to the output stream given to the constructor. This
   /// produces essentially a complete *.h file to interface with a compiled
   /// module.
   ///
   /// module: The HILTI code to generate the prototypes for. This must have
   /// been verified and resolved already.
   void generatePrototypes(shared_ptr<hilti::Module> module);

   // Returns a string with the C type corresponding to a given HILTI type.
   string mapType(shared_ptr<Type> type) { return type_mapper.mapType(type); }

protected:
   std::ostream& output() const { return _output; }

   void visit(type::Enum* c) override;
   void visit(declaration::Function* f) override;

private:
   std::ostream& _output;
   protogen::TypeMapper type_mapper;
};

}
}

#endif
