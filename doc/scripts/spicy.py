# -*- coding: utf-8 -*-
"""X
    The Spicy domain for Sphinx.
"""

def setup(Sphinx):
    Sphinx.add_domain(SpicyDomain)

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

class SpicyGeneric(ObjectDescription):
    def add_target_and_index(self, name, sig, signode):
        targetname = self.objtype + '-' + name
        if targetname not in self.state.document.ids:
            signode['names'].append(targetname)
            signode['ids'].append(targetname)
            signode['first'] = (not self.names)
            self.state.document.note_explicit_target(signode)

            objects = self.env.domaindata['spicy']['objects']
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

class SpicyOperator(SpicyGeneric):
    def handle_signature(self, sig, signode):
        m = sig.split()
        name = m[0]
        args = m[1:] if len(m) > 1 else []

        for a in args:
            if a.startswith("t:"):
                signode += addnodes.literal_emphasis("", a[2:])

            elif a.startswith("a:"):
                signode += addnodes.literal_emphasis("", a[2:])

            elif a.startswith("op:"):
                signode += nodes.literal("", a[3:])

            elif a.startswith("x:"):
                signode += nodes.emphasis("", a[2:].replace("-", " "))

            elif a == "<sp>":
                signode += nodes.inline("", " ")

            else:
                signode += nodes.inline("", a)

        return name

class SpicyMethod(SpicyGeneric):
    def handle_signature(self, sig, signode):
        m = sig.split()
        name = m[0]
        result = m[1]
        func = m[2]
        args = sig[sig.find("(") + 1:-1]

        if result != "-":
            try:
                (ns, id) = result.split("::")
                rnode = addnodes.pending_xref("", refdomain='spicy', reftype='type', reftarget=result)
                rnode += nodes.literal("", id, classes=['xref'])

            except ValueError:
                rnode = nodes.inline("", result + "X")

            signode += rnode
            signode += nodes.inline("", " ")

        signode += nodes.strong("", func)
        signode += nodes.strong("", "(")
        signode += nodes.literal("", args)
        signode += nodes.strong("", ")")
        return name

class SpicyType(SpicyGeneric):
    def handle_signature(self, sig, signode):
        name = sig

        if sig.find("::") > 0:
            signode += nodes.strong("", name)

        return name

class SpicyFunction(SpicyGeneric):
    def handle_signature(self, sig, signode):
        name = sig

        if sig.find("::") > 0:
            signode += nodes.strong("", name)

        return name

class SpicyMethodXRefRole(XRefRole):
    def process_link(self, env, refnode, has_explicit_title, title, target):
        i = title.find("::")

        if i > 0 :
            title = title[i+2:] + "()"

        return title, target

class SpicyDomain(Domain):
    """Spicy domain."""
    name = 'spicy'
    label = 'Spicy'

    object_types = {
        'operator':    ObjType(l_('operator'), 'op'),
        'method':      ObjType(l_('method'),   'method'),
        'type':        ObjType(l_('type'),     'type'),
        'function':    ObjType(l_('function'), 'function'),
    }

    directives = {
        'operator':      SpicyOperator,
        'method':        SpicyMethod,
        'type':          SpicyType,
        'function':      SpicyFunction,
    }

    roles = {
        'op':         XRefRole(),
        'method':     SpicyMethodXRefRole(),
        'type':       XRefRole(),
        'function':   XRefRole(),
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
