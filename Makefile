#--------------------------------------
# Makefile for all Recommender packages
#--------------------------------------

default:
	- ( cd src && $(MAKE) all )
	- ( cd tools/kfold && $(MAKE) all )
	- ( cd tools/serialization && $(MAKE) all )
	- ( cd tools/webserver && $(MAKE) all )
all:
	- ( cd src && $(MAKE) all )
	- ( cd tools/kfold && $(MAKE) all )
	- ( cd tools/webserver && $(MAKE) all )

purge:
	- ( cd src && $(MAKE) purge )

clean:
	- ( cd src && $(MAKE) clean )
	- ( cd tools/kfold && $(MAKE) clean )
	- ( cd tools/serialization && $(MAKE) clean )
	- ( cd tools/webserver && $(MAKE) clean )
