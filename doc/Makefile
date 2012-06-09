
# Note: Doxygen is configured to write to _build/doxygen.
BUILDDIR = build

all: mkdir autogen doxygen sphinx

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
	../build/tools/hilti-doc | ../scripts/doc-to-rst $(BUILDDIR)/autogen

clean:
	rm -rf $(BUILDDIR)

