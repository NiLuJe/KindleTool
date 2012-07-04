# Toplevel Makefile, all the fun stuff happens in KindleTool/Makefile ;)

default: all

all:
	$(MAKE) -C KindleTool all

kindle:
	$(MAKE) -C KindleTool kindle

mingw:
	$(MAKE) -C KindleTool mingw

debug:
	$(MAKE) -C KindleTool debug

strip:
	$(MAKE) -C KindleTool strip

clean:
	$(MAKE) -C KindleTool clean

install:
	$(MAKE) -C KindleTool install
