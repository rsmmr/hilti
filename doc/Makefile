
# Note: Doxygen is configured to write to _build/doxygen.
BUILDDIR = build

all: mkdir doxygen sphinx

mkdir:
	test -d || mkdir -p build

doxygen:
	doxygen

sphinx:
	sphinx-build -b html . $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

