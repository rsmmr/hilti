
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
  
  // auto functions = getFunctions(_module);
  
  // for ( auto fdata : *functions )
  //   {
  //     string fName = fdata.first->function()->id()->name();
  //     if ( _FP_DP )
  // 		std::cerr << "Now analyzing " << fName << std::endl;

  //     if ( fName.find("parse_") == 0 && fName.rfind("_internal") == fName.length()-9 )
  // 		std::cerr << fName << " is the internal parsing function!!!" << std::endl;
  //   }

  
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
  
  processAllPostOrder(_module);
  assert( _structNode != nullptr && _structType != nullptr );
}


void FieldPruning::visit(type::Struct* t)
{
  auto tPtr = t->sharedPtr<type::Struct>();

  if ( tPtr->id()->name() == _structName ) {
	assert( _structType == nullptr );
	_structType = tPtr;
	if ( _FP_DBG )
	  std::cerr << "State struct type found!" << std::endl;
  }
}

void FieldPruning::visit(variable::Local* v)
{
  auto vPtr = v->sharedPtr<variable::Local>();
  std::cerr << "Visiting local " << vPtr->id()->name() << std::endl;

  auto vType = ast::checkedCast<Type>(vPtr->type());
  if ( vType->id()->name() == _structType->id()->name() )
	{
	  assert( _structNode == nullptr);
	  _structNode = vPtr;
	  if ( _FP_DBG )
		std::cerr << "State struct type found!" << std::endl;
	}
}


// shared_ptr<FieldPruning::function_list> FieldPruning::getFunctions(shared_ptr<Module> _module)
// {
//   auto retval = std::make_shared<FieldPruning::function_list>();

//   if ( _module->body() )
//     {
//       auto _mbody = ast::checkedCast<statement::Block>(_module->body());

//       for ( auto _decl : _mbody->declarations() )
//         {
//           shared_ptr<declaration::Function> _fdecl;
//           if ( (_fdecl = ast::tryCast<declaration::Function>(_decl)) != NULL )
//             {
//               if ( _fdecl->function()->body() )
//                 {
//                   shared_ptr<statement::instruction::Resolved> _insn = nullptr;
//                   auto _fbody = ast::checkedCast<statement::Block>(_fdecl->function()->body());
                  
//                   do {
//                     _insn = ast::tryCast<statement::instruction::Resolved>(_fbody->statements().front());
//                     if ( _insn == NULL )
//                       _fbody = ast::checkedCast<statement::Block>(_fbody->statements().front());
//                   } while ( _insn == NULL );

//                   retval->push_back(function_info(_fdecl, _insn));
//                 }
//             }
//         }
//     }

//   return retval;
// }
