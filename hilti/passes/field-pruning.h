// Local Variables:
// mode: c++
// End:

#ifndef HILTI_PASSES_FIELD_PRUNING_H
#define HILTI_PASSES_FIELD_PRUNING_H

#include "../pass.h"

namespace hilti {

class CompilerContext;

namespace passes {

class CFG;


class FieldPruning : public Pass<>
{
public:

  FieldPruning(CompilerContext* context, shared_ptr<CFG> cfg);
  virtual ~FieldPruning();

  bool run(shared_ptr<Node> module) override;
  
private:

  void findSessionStruct(shared_ptr<Module> _module);
  void visit(type::Struct* t) override;
  void visit(variable::Local* v) override;
  
  CompilerContext* _context;
  shared_ptr<Module> _module = nullptr;
  shared_ptr<CFG> _cfg;
  std::string _structName;
  shared_ptr<type::Struct> _structType;
  shared_ptr<variable::Local> _structNode;

  bool _FP_DBG = true;
};

} // namespace passes
} // namespace hilti

#endif
