SUBDIRS = src

EXTRA_DIST = scripts/NeuroServer.iss.in

#src/neurod:
#	(cd src ; make)
dox:
	rm -rf html ; doxygen

clean:
	rm -rf html ; cd src ; make clean

dclean:
	rm -rf .deps src/.deps
