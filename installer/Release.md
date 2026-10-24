# Release procedure for ep128emu:


- update licenses if needed for the code in deps/
- bump version in ep128emu.hpp and resources/ep128emu.rc
- add new configuration templates to makecfg.cpp if relevant
- pack required roms: epcompress -a -m2 @romlist.txt ep128emu_roms-<version>.bin
- update version and rom file in ep128emu.nsi
- build Linux
- tar cvfz ep128emu-2.0.12.7-x64.tar.xz config/ disk/ licenses/ roms/*.rom NEWS README PKGBUILD COPYING ep128emu epcompress epimgconv epmakecfg tapeedit
- build win32 (old)
- build installer: wine C:\\Program\ Files\ \(x86\)\\NSIS\\makensis.exe ep128emu.nsi
- build win64 (old)
- build installer: wine C:\\Program\ Files\ \(x86\)\\NSIS\\makensis.exe /DWIN64 ep128emu.nsi
- create github release, upload the files
- upload rompack to zoltanvb.github.io, update link
