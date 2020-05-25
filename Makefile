# Old method, broken by some change to msbuild?
# MSBUILD=/c/Program\ Files\ \(x86\)/MSBuild/12.0/Bin/MSBuild.exe
MSBUILD=./msbuild.workaround

NSIS="/c/Program Files (x86)/NSIS/makensis.exe"

# The NSIS from msys2 builds 64 bit installers that aren't compatible with 32 bit systems.
# NSIS=makensis

DEPLOYPATH="/d/Other games/SMAC-git"

.PHONY: pracx installer deploy test testpath clean

all: pracx installer

pracx: bin/prax.dll bin/prac.dll

bin/prac.dll bin/prax.dll bin/pracxpatch.exe: $(shell find shared pracxpatch -type f)
	$(MSBUILD) pracx.sln /v:m /m /p:Configuration=Release
	# MSBUILD won't touch the timestamps if it doesn't need to update the
	# files, which confuses make.
	touch bin/prac.dll bin/prax.dll bin/pracxpatch.exe

installer: pracx
	$(NSIS) //V1 InstallScript/PRACX.nsi

deploy: pracx
	cp bin/prax.dll bin/prac.dll bin/pracxpatch.exe resources/Icons.pcx $(DEPLOYPATH)

test: pracx deploy
	bash -c 'cd $(DEPLOYPATH);sed 's/DisableOpeningMovie=0/DisableOpeningMovie=1/' Alpha\ Centauri.Ini -i; cmd //K terranx <<< "exit"' 

testpatch: bin/pracxpatch deploy
	bash -c 'cd $(DEPLOYPATH); ./pracxpatch'

clean:
	rm -rf obj/* bin/* Debug/*
