SHELL := /bin/bash

# NSIS="/c/Program Files (x86)/NSIS/makensis.exe"
NSIS=makensis

DEPLOYPATH="/d/Other games/SMAC-git"

.PHONY: pracx installer deploy test testpatch clean resetdeployment loader-gcc

all: pracx installer

pracx: bin/prax.dll bin/prac.dll bin/loader.dll bin/pracxpatch.exe

bin/prax.dll: Makefile shared/pracx.cpp shared/pracxsettings.* shared/wm2str.cpp shared/terran.h
	mkdir -p build/prax bin
	source ./vsenv && cl -nologo -LD -EHsc shared/pracx.cpp shared/pracxsettings.cpp shared/wm2str.cpp -Fo"build/prax/" -Fe"bin/prax.dll"

bin/prac.dll: Makefile shared/pracx.cpp shared/pracxsettings.* shared/wm2str.cpp shared/terran.h
	mkdir -p build/prac bin
	source ./vsenv && cl -nologo -LD -EHsc shared/pracx.cpp shared/pracxsettings.cpp shared/wm2str.cpp -Fo"build/prac/" -D_SMAC -Fe"bin/prac.dll"

bin/loader.dll: Makefile loader/loader.*
	mkdir -p build/loader bin
	source ./vsenv && cl -nologo loader/loader.cpp -DLOADER_EXPORTS -EHsc -LD -Fo"build/loader/" -Febin/loader.dll -link -DEF:loader/loader.def

loader-gcc: Makefile loader/*
	mkdir -p build/loader bin
	codeblocks --rebuild loader.cbp --target=Release

bin/pracxpatch.exe: Makefile pracxpatch/pracxpatch.cpp
	mkdir -p build/pracxpatch bin
	cl -nologo pracxpatch/pracxpatch.cpp -Fo"build/pracxpatch/" -Febin/pracxpatch.exe

installer: pracx resources/*
	$(NSIS) //V1 InstallScript/PRACX.nsi

clean:
	rm -rf build bin

deploy: pracx
	cp bin/*.dll bin/pracxpatch.exe resources/Icons.pcx $(DEPLOYPATH)

test: pracx deploy
	cd $(DEPLOYPATH) \
		&& sed 's/DisableOpeningMovie=0/DisableOpeningMovie=1/' Alpha\ Centauri.Ini -i \
		&& cmd /c terranx

testsmac: pracx deploy
	cd $(DEPLOYPATH) \
		&& sed 's/DisableOpeningMovie=0/DisableOpeningMovie=1/' Alpha\ Centauri.Ini -i \
		&& cmd /c terran

testpatch: bin/pracxpatch.exe deploy
	cd $(DEPLOYPATH) && cmd /c pracxpatch

resetdeployment:
	cd $(DEPLOYPATH) && git reset --hard
