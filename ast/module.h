
#ifndef AST_MODULE_H
#define AST_MODULE_H

#include <set>

#include "exception.h"
#include "node.h"
#include "id.h"
#include "scope.h"

namespace ast {

/// An AST node for a top-level module. A module has a name and a top-level
/// body statement that's assumed to be executed at module-initialization
/// time, outside of any functions. By choosing a composite statement type
/// for the latter, this can actually be a series of statements. The module
/// does not any Declarartion itself, but these can likewise be part of the
/// statement. Typically, statement::Block is the one you'll want to use
/// here.
template<typename AstInfo>
class Module : public AstInfo::node
{
public:
   typedef typename AstInfo::node Node;
   typedef typename AstInfo::body_statement BodyStatement;
   typedef typename AstInfo::function Function;
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::id ID;

   /// Constructor.
   ///
   /// id: A non-scoped ID with the module's name.
   ///
   /// path: A file system path associated with the module.
   ///
   /// l: Associated location.
   Module(shared_ptr<ID> id, const string& path = "-", const Location& l=Location::None) : Node(l) {
       assert(! id->isScoped());
       _id = id;
       _path = path;
       this->addChild(_id);
   }

   /// Returns the module's name.
   shared_ptr<ID> id() const { return _id; }

   /// Returns the file system path associated with the module.
   const string& path() const { return _path; }

   /// Returns the module's body statement.
   shared_ptr<BodyStatement> body() const { return _body; }

   /// Sets the module's body statement.
   ///
   /// body: The statement.
   void setBody(shared_ptr<BodyStatement> body) {
       if ( _body )
           this->removeChild(_body);

       _body = body;
       this->addChild(_body);
   }

   /// Imports an ID into the module's state. The AST node does not do more
   /// than just recording the fact that the ID is imported. importedID()
   /// then provides acces to the list of all importaed IDs to other
   /// components.
   ///
   /// id: The imported ID.
   void import(shared_ptr<ID> id) {
       auto n = node_ptr<ID>(id);
       _imported.push_back(n);
       this->addChild(n);
   }

   /// Imports an ID into the module's state. The AST node does not do more
   /// than just recording the fact that the ID is imported. importedID()
   /// then provides acces to the list of all importaed IDs to other
   /// components.
   ///
   /// id: The imported name.
   void import(string id) {
       import(shared_ptr<ID>(new ID(id)));
   }

   typedef std::list<node_ptr<ID>> id_list;
   typedef std::list<node_ptr<Type>> type_list;

   /// Returns the list of all IDs imported with importedID().
   const id_list& importedIDs() const { return _imported; }

   /// Exports an ID for access by other modules.  The AST node does not do
   /// more than just recording the fact that the ID is exported. exported()
   /// and exportedID() can then be used by other components to get access to
   /// that information.
   ///
   /// id: The exported ID.
   ///
   /// implicit: True if this an implicitly exported ID. It will accordingly
   /// be included (or not) by exportedIDs().
   void exportID(shared_ptr<ID> id, bool implicit=false) {
       _exported_ids.push_back(std::make_pair(id, implicit));
       _exported.insert(id->name());
   }

   /// Exports an ID for access by other modules.  The AST node does not do
   /// more than just recording the fact that the ID is exported. exported()
   /// and exportedID() can then be used by other components to get access to
   /// that information.
   ///
   /// id: The exported name.
   void exportID(const string& name, bool implicit=false) {
       auto id = shared_ptr<ID>(new ID(name));
       exportID(id, implicit);
   }

   /// Exports a Type for access by other modules.  The AST node does not do
   /// more than just recording the fact that the type is exported. 
   /// exportedTypes() can then be used by other components to get access to
   /// that information.
   ///
   /// type: The exported type.
   void exportType(shared_ptr<Type> type) {
       _exported_types.push_back(type);
   }

   /// Exports a Type for access by other modules.  The AST node does not do
   /// more than just recording the fact that the type is exported. 
   /// exportedTypes() can then be used by other components to get access to
   /// that information.
   ///
   /// type: The exported type.
   bool exported(shared_ptr<ID> id) {
       return _exported.find(id->name()) != _exported.end();
   }

   /// Returns the list of IDs exported with exportID().
   ///
   /// implicit: True if implicitly supported IDs are to be included.
   const id_list exportedIDs(bool implicit) const {
       id_list ids;

       for ( auto id : _exported_ids )
           if ( id.second == false || implicit )
               ids.push_back(id.first);

       return ids;
   }

   /// Returns the list of types exported with exportType().
   const type_list& exportedTypes() const { return _exported_types; }

private:
   node_ptr<ID> _id;
   string _path;
   node_ptr<BodyStatement> _body;
   id_list _imported;

   typedef std::list<std::pair<node_ptr<ID>, bool>> internal_id_list;
   internal_id_list _exported_ids;
   type_list _exported_types;

   typedef std::set<string> string_set;
   string_set _exported;

   typedef std::list<node_ptr<Function>> function_list;
   function_list _functions;
};

}

#endif
