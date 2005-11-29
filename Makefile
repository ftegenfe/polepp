ROOTCFLAGS = $(shell root-config --cflags)
ROOTLIBS   = $(shell root-config --libs)
LDOBJS     =  Pdf.o Random.o Coverage.o Pole.o Combination.o Combine.o Observable.o Power.o
LDSMOBJS    =  Pdf.o Random.o
LDFLAGS    = -g -fPIC -shared
###CFLAGS     = -O
# Use -g to include debug info
# Use -pg for profiling info
CFLAGS     = -g -O
SRCTOOLS     = polelim.cxx polecov.cxx exptest.cxx plotexp.cxx estbelt.cxx polebelt.cxx poleconst.cxx polepow.cxx
INCFILES     = Pole.h Coverage.h Random.h Range.h Pdf.h BeltEstimator.h Combination.h Measurement.h Power.h
SRCFILES     = Pole.cxx Coverage.cxx Random.cxx Pdf.cxx Combination.cxx Measurement.cxx Power.cxx
LDFILE	     = libPole++.so.1
LDSMFILE     = libPoleSM++.so
LIBS         = -L./ -lPole++
LIBSSM       = -L./ -lPoleSM++

all:		$(LDFILE) polelim polebelt polecov exptest plotexp estbelt poleconst polecomb polepow
small:          $(LDSMFILE)

$(LDFILE):	$(LDOBJS)
		g++ $(LDFLAGS) $(LDOBJS) -o $(LDFILE)

$(LDSMFILE):	$(LDSMOBJS)
		g++ $(LDFLAGS) $(LDSMOBJS) -o $(LDSMFILE)

polepow:	polepow.o
		g++ $(CFLAGS) -Wall $< $(LIBS)  -o $@
polepow.o:	polepow.cxx
		g++ $(CFLAGS) -Wall -c $<

polecomb:       polecomb.o
		g++ $(CFLAGS) -Wall $< $(LIBS)  -o $@
polecomb.o:     polecomb.cxx
		g++ $(CFLAGS) -Wall -c $<

estbelt:	estbelt.o
		g++ $(CFLAGS) -Wall $< $(LIBS)  -o $@
estbelt.o:	estbelt.cxx
		g++ $(CFLAGS) -Wall -c $<

poleconst:	poleconst.o
		g++ $(CFLAGS) -Wall $< $(LIBS)  -o $@
poleconst.o:	poleconst.cxx
		g++ $(CFLAGS) -Wall -c $<

polelim:	polelim.o
		g++ $(CFLAGS) -Wall $< $(LIBS)  -o $@
polelim.o:	polelim.cxx
		g++ $(CFLAGS) -Wall -c $<

polebelt:	polebelt.o
		g++ $(CFLAGS) -Wall $< $(LIBS)  -o $@
polebelt.o:	polebelt.cxx
		g++ $(CFLAGS) -Wall -c $<

polecov:	polecov.o
		g++ $(CFLAGS) -Wall $< $(LIBS)  -o $@
polecov.o:	polecov.cxx
		g++ $(CFLAGS) -Wall -c $<

exptest:	exptest.o
		g++ $(CFLAGS) -Wall $< $(LIBS)  -o $@
exptest.o:	exptest.cxx
		g++ $(CFLAGS) -Wall -c $<

plotexp:	plotexp.o
		g++ $(CFLAGS) -Wall $< $(ROOTLIBS)  -o $@
plotexp.o:	plotexp.cxx
		g++ $(CFLAGS) $(ROOTCFLAGS) -Wall -c $<

obstest:	obstest.o Observable.o
		g++ $(CFLAGS) -Wall $< $(LIBS)  -o $@
obstest.o:	obstest.cxx
		g++ $(CFLAGS) -Wall -c $<

pdftst:		pdftst.o PdfNew.o
		g++ $(CFLAGS) -Wall $< PdfNew.o $(LIBS)  -o $@
pdftst.o:	pdftst.cxx PdfNew.h
		g++ $(CFLAGS) -Wall -c $<

pdftstold:	pdftstold.o
		g++ $(CFLAGS) -Wall $< $(LIBS)  -o $@
pdftstold.o:	pdftstold.cxx
		g++ $(CFLAGS) -Wall -c $<

meastst:	meastst.o
		g++ $(CFLAGS) -Wall $< $(LIBSSM)  -o $@
meastst.o:	meastst.cxx
		g++ $(CFLAGS) -Wall -c $<

Combine.o:      Combine.cxx Combine.h
		g++ $(CFLAGS) -Wall -c $<

Pole.o:		Pole.cxx Pole.h Pdf.h Range.h
		g++ $(CFLAGS) -Wall -c $<
Coverage.o:	Coverage.cxx Coverage.h Range.h Random.h Pole.h Pdf.h
		g++ $(CFLAGS) -Wall -c $<
Random.o:	Random.cxx Random.h
		g++ $(CFLAGS) -Wall -c $<
Pdf.o:		Pdf.cxx Pdf.h
		g++ $(CFLAGS) -Wall -c $<
Combination.o:	Combination.cxx Combination.h
		g++ $(CFLAGS) -Wall -c $<
Observable.o:	Observable.cxx Observable.h Random.h
		g++ $(CFLAGS) -Wall -c $<
Measurement.o:	Measurement.cxx Measurement.h
		g++ $(CFLAGS) -Wall -c $<
Power.o:	Power.cxx Power.h
		g++ $(CFLAGS) -Wall -c $<

package:	$(SRCLIB) $(SRCTOOLS) Makefile
		tar -czf polelib.tgz $(SRCFILES) $(INCFILES) $(SRCTOOLS) Makefile

clean:
		rm -f *.o
