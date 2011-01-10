
WWW = $(HOME)/www/btest

all:

.PHONY: dist

dist: 
	python setup.py sdist

www: dist
	rst2html.py README >$(WWW)/index.html
	cp dist/* $(WWW)
	# cp CHANGES $(WWW)
