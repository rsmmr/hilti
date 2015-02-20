
#include "hilti/hilti-intern.h"
#include "field-pruning.h"


using namespace hilti::passes;

FieldPruning::FieldPruning(CompilerContext* context, shared_ptr<CFG> cfg) : Pass<>("hilti::FieldPruning")
{
  _context = context;
  _cfg = cfg;
}


FieldPruning::~FieldPruning()
{
}


bool FieldPruning::run(shared_ptr<Node> module)
{
  _module = ast::checkedCast<Module>(module);

  findSessionStruct(_module);
  

  return true;
}


// shared_ptr<variable::Global> getSessionStruct(shared_ptr<Module> _module)
void FieldPruning::findSessionStruct(shared_ptr<Module> _module)
{
  _structName = "";
  _structNode = nullptr;
  _structType = nullptr;
  
  for ( auto id : _module->exportedIDs(false) )
    {      
      if ( id->name().find("parse_") == 0 )
		{
		  assert( id->name().length() > 6);
		  _structName = id->name().substr(6);
		  if ( _FP_DBG )
			std::cerr << "State struct name is " << id->name() << std::endl;
		  break;
		}
    }
  assert( _structName != "" );

  // The idea here is to find the declaration of a struct named
  // "__tmp_pobj" whose type name is equal to the one inferred above
  processAllPostOrder(_module);
  assert( _structNode != nullptr && _structType != nullptr );
}


void FieldPruning::visit(declaration::Variable* d)
{
  auto dPtr = d->sharedPtr<declaration::Variable>();

  if ( dPtr->id()->name() == "__tmp_pobj" )
	{
	  // Get the variable
	  auto vPtr = ast::checkedCast<variable::Local>(dPtr->variable());
	  if ( vPtr != nullptr )
		{
		  // Get reference type
		  auto rPtr = ast::checkedCast<type::Reference>(vPtr->type());
		  if ( rPtr != nullptr )
			{
			  auto tPtr = ast::checkedCast<type::Struct>(rPtr->argType());
			  if ( tPtr->id()->name() == _structName && _structNode == nullptr )
				{
				  _structNode = vPtr;
				  _structType = tPtr;
				  std::cerr << "Find var " << vPtr->id()->name() << " of type ref<"
							<< tPtr->id()->name() << "> !!" << std::endl;
				}
			}
		}
	}
}
