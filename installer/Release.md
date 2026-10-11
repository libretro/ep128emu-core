# Release procedure for ep128emu:


- update licenses if needed for the code in deps/
- bump version in gui/gui.cpp and resources/ep128emu.rc
- pack required roms: epcompress -a -m2 @romlist.txt ep128emu_roms-<version>.bin
- upload to zoltanvb.github.io, update link
- update version and rom file in ep128emu.nsi
- build
- build installer: makensis.exe /DWIN64 ep128emu.nsi
