# Old method, broken by some change to msbuild?
# MSBUILD=/c/Program\ Files\ \(x86\)/MSBuild/12.0/Bin/MSBuild.exe
MSBUILD=./msbuild.workaround

# NSIS="/c/Program Files (x86)/NSIS/makensis.exe"
NSIS=makensis

DEPLOYPATH="/d/Other games/SMAC-git"

.PHONY: pracx installer deploy test testpath clean resetdeployment

all: pracx installer

pracx: bin/prax.dll bin/prac.dll

bin/loader.dll bin/prac.dll bin/prax.dll bin/pracxpatch.exe: $(shell find shared pracxpatch -type f) Makefile
	$(MSBUILD) pracx.sln /v:m /m /p:Configuration=Release
	# MSBUILD won't touch the timestamps if it doesn't need to update the
	# files, which confuses make.
	touch bin/*.dll bin/pracxpatch.exe

installer: pracx
	$(NSIS) //V1 InstallScript/PRACX.nsi

deploy: pracx
	cp bin/*.dll bin/pracxpatch.exe resources/Icons.pcx $(DEPLOYPATH)

test: pracx deploy
	bash -c 'cd $(DEPLOYPATH);sed 's/DisableOpeningMovie=0/DisableOpeningMovie=1/' Alpha\ Centauri.Ini -i; cmd //K terranx <<< "exit 0"' 

testpatch: bin/pracxpatch.exe deploy
	bash -c 'cd $(DEPLOYPATH); ./pracxpatch'

clean:
	rm -rf obj/* bin/* Debug/*

resetdeployment:
	bash -c 'cd $(DEPLOYPATH); git reset --hard'

bin/loader.dll: shared/loader.h shared/loader.cpp Makefile
	$(MSBUILD) loader/loader.vcxproj /v:m /m /p:Configuration=Release
	# g++ -o bin/loader.o -c shared/loader.cpp -m32 -std=c++11 -pedantic -Wall -Wextra -static-libgcc -static-libstdc++
	# g++ -shared -o bin/load.dll bin/loader.o -m32 -std=c++11 -static-libgcc -static-libstdc++
