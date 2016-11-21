# -*- coding: utf-8 -*-
"""X
    The Hlt domain for Sphinx.
"""

def setup(Sphinx):
    Sphinx.add_domain(HltDomain)

from sphinx import addnodes
from sphinx.domains import Domain, ObjType
from sphinx.locale import l_, _
from sphinx.directives import ObjectDescription
from sphinx.roles import XRefRole
from sphinx.util.nodes import make_refnode
from sphinx import version_info

from docutils import nodes

# Wrapper for creating a tuple for index nodes, staying backwards
# compatible to Sphinx < 1.4:
def make_index_tuple(indextype, indexentry, targetname, targetname2):
    if version_info >= (1, 4, 0, '', 0):
        return (indextype, indexentry, targetname, targetname2, None)
    else:
        return (indextype, indexentry, targetname, targetname2)

class HltGeneric(ObjectDescription):
    def add_target_and_index(self, name, sig, signode):
        targetname = self.objtype + '-' + name
        if targetname not in self.state.document.ids:
            signode['names'].append(targetname)
            signode['ids'].append(targetname)
            signode['first'] = (not self.names)
            self.state.document.note_explicit_target(signode)

            objects = self.env.domaindata['hlt']['objects']
            key = (self.objtype, name)
            if key in objects:
                self.env.warn(self.env.docname,
                              'duplicate description of %s %s, ' %
                              (self.objtype, name) +
                              'other instance in ' +
                              self.env.doc2path(objects[key]),
                              self.lineno)
            objects[key] = self.env.docname
        indextext = self.get_index_text(self.objtype, name)
        if indextext:
            self.indexnode['entries'].append(make_index_tuple('single', indextext,
                                              targetname, targetname))

    def get_index_text(self, objectname, name):
        return _('%s (%s)') % (name, self.objtype)

    def handle_signature(self, sig, signode):
        signode += addnodes.desc_name("", sig)
        return sig

class HltInstruction(HltGeneric):
    def handle_signature(self, sig, signode):
        m = sig.split()
        cls = m[0]
        name = m[1]
        args = m[2:] if len(m) > 2 else []

        if len(args) > 1 and args[0] == "target":
            args = args[1:] if len(args) > 1 else []
            signode += addnodes.desc_addname("", "target = ")

        args = " ".join(args)
        args = " " + args
        desc_name = name
        signode += addnodes.desc_name("", desc_name)
        if len(args) > 0:
            signode += addnodes.desc_addname("", args)

        signode += addnodes.desc_addname("", " [%s]" % cls)

        return name

class HltType(HltGeneric):
    def handle_signature(self, sig, signode):
        # Do nothing, we just want an anchor for xrefing.
        m = sig.split()
        name = m[0]
        return name

class HltTypeDef(HltGeneric):
    def handle_signature(self, sig, signode):
        m = sig.split()
        full = m[0]
        short = m[1]
        ty = m[2]

        signode += addnodes.desc_addname("", ty + " ")
        signode += addnodes.desc_name("", short)
        return full

class HltGlobal(HltGeneric):
    def handle_signature(self, sig, signode):
        m = sig.split()
        full = m[0]
        short = m[1]
        signode += addnodes.desc_name("", short)
        return full

class HltModule(HltGeneric):
    def handle_signature(self, sig, signode):
        # Do nothing, we just want an anchor for xrefing.
        m = sig.split()
        name = m[0]
        return name

class HltDomain(Domain):
    """Hlt domain."""
    name = 'hlt'
    label = 'HILTI'

    object_types = {
        'instruction': ObjType(l_('instruction'), 'ins'),
        'operator':    ObjType(l_('operator'),    'op'),
        'overload':    ObjType(l_('overload'),    'ovl'),
        'type':        ObjType(l_('type'),        'type'),
        'typedef':     ObjType(l_('typedef'),     'typedef'),
        'function':    ObjType(l_('function'),    'func'),
        'global':      ObjType(l_('global'),      'glob'),
        'module':      ObjType(l_('module'),      'mod'),
    }

    directives = {
        'instruction':   HltInstruction,
        'operator':      HltInstruction,
        'overload':      HltInstruction,
        'type':          HltType,
        'typedef':       HltTypeDef,
        'function':      HltGeneric,
        'global':        HltGlobal,
        'module':        HltModule,
    }

    roles = {
        'ins':    XRefRole(),
        'op':     XRefRole(),
        'ovl':    XRefRole(),
        'type':   XRefRole(),
        'func':   XRefRole(),
        'glob':   XRefRole(),
        'mod':    XRefRole(),
    }

    initial_data = {
        'objects': {},  # fullname -> docname, objtype
    }

    def clear_doc(self, docname):
        for (typ, name), doc in self.data['objects'].items():
            if doc == docname:
                del self.data['objects'][typ, name]

    def resolve_xref(self, env, fromdocname, builder, typ, target, node,
                     contnode):
        objects = self.data['objects']
        objtypes = self.objtypes_for_role(typ)
        for objtype in objtypes:
            if (objtype, target) in objects:
                return make_refnode(builder, fromdocname,
                                    objects[objtype, target],
                                    objtype + '-' + target,
                                    contnode, target + ' ' + objtype)

    def get_objects(self):
        for (typ, name), docname in self.data['objects'].items():
            yield name, name, typ, docname, typ + '-' + name, 1
