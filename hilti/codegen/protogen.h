
#ifndef HILTI_CODEGEN_PROTOGEN_H
#define HILTI_CODEGEN_PROTOGEN_H

#include <fstream>

#include "../common.h"
#include "../visitor.h"

#include "common.h"

namespace hilti {
namespace codegen {

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

protected:
   std::ostream& output() const { return _output; }

   void visit(type::Enum* c) override;

private:
   std::ostream& _output;
};

}
}

#endif
