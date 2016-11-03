
# Note: Doxygen is configured to write to _build/doxygen.
BUILDDIR = build

all: mkdir autogen sphinx

mkdir:
	test -d $(BUILDDIR)                 || mkdir -p $(BUILDDIR)
	test -d $(BUILDDIR)/autogen         || mkdir -p $(BUILDDIR)/autogen
	test -d $(BUILDDIR)/autogen/doxygen || mkdir -p $(BUILDDIR)/autogen/doxygen
	test -d $(BUILDDIR)/html            || mkdir -p $(BUILDDIR)/html

doxygen:
	doxygen

sphinx:
	sphinx-build -b html . $(BUILDDIR)/html

autogen:
	( ../build/tools/hilti-doc; ../build/tools/spicy-doc ) | ../scripts/doc-to-rst $(BUILDDIR)/autogen

clean:
	rm -rf $(BUILDDIR)

install-www: all
	scp -rp build/html/* www.icir.org:hilti

