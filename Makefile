MSBUILD=/c/Program\ Files\ \(x86\)/MSBuild/12.0/Bin/MSBuild.exe
NSIS="/c/Program Files (x86)/NSIS/makensis.exe"
all:
	$(MSBUILD) pracx.sln //v:m //p:Configuration=Release

installer: all
	$(NSIS) //V2 InstallScript/PRACX.nsi
