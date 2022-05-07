 Update May 7, 2022

 PLUGIN SETUP:
 =============

 WiiFlow can display and launch games from plugin apps (other homebrew apps that launches other type of games).
 Each plugin must have its own ini file declared in this folder. This file describes how to set up those files.
 
 Wii, Wii channels, Gamecube and Homebrew games may also be declared as plugins, in order to have them mixed
 with other game types in coverflow, refer to wiiflow/plugins_data/platform.ini to get their proper magic #.

 Each plugin can have its own "controller input guide", which is simply an image displayed in game menu if guidename= is defined in ini file (see below). PNG or JPG must be placed in wiiflow/plugins/inputs/

 Also note a plugin pack has been created by Tetsuo Shima :
	https://gbatemp.net/threads/the-great-quest-for-wiiflow-plugins-tm-a-call-for-adventurers.563575/

 
 PLUGIN INI FILES SETTINGS:
 ==========================

[PLUGIN]
displayname=
# name to show in WiiFlow plugin select menu

magic=
# plugin magic number (8 digit ASCII to HEX name - refer to wiiflow/plugins_data/platform.ini)

dolfile=
# homebrew DOL app to boot (can be [plugin].dol in plugin folder or apps/[Homebrew folder]/boot.dol)
# use the keyword "music" to make WiiFlow act as a music player (files read are .pls, .m3u, .mp3, .ogg)

romdir=
# rom directory (without device)
# for ScummVM plugin: full path of scummvm.ini file with device and extensions if not in plugin folder
# (can be scummvm.ini in plugin folder or sd:/apps/scummvm/scummvm.ini)

arguments=
# arguments sent to the plugin app (use: {device}:/{path}|{name}|{loader} for most plugins)

filetypes=
# file extensions to look for, separated by pipes (e.g: .nes|.fds)
# use .cue for CD based games

coverfolder=
# boxcovers / covers subfolder in which PNG cover images are stored

rompartition=
# optional - specific device: (0) for SD, (1) for USB, (-1 or blank) for default

boxmode=
# optional - covers in box style: (0) for off (flat), (1) for on, (-1 or blank) for default

covercolor=
# optional - cover spine color in hexadecimal: 000000 black (default), ff0000 red, ffffff white, fcff00 yellow, 01a300 green, and 111111 for clear cd case

bannersound=
# optional - sound played whenever you select a game associated with this plugin (e.g. atari.ogg)

consolecoverid=
# optional - for access to web cover database - currently not used

explorerpath=
# optional - rom directory to explore in file explorer view (device must be specified: "sd:/" or "usb1:/"), default to "romdir" value

filenamesastitles=
# optional (emulators only) - force WiiFlow to use rom file names as titles instead of Wiimpathy's database (yes/no - default to no)

guidename=
# optional - background image (e.g. "atari_guide.png") to use for the plugin controller input guide (PNG or JPG must be placed in wiiflow/plugins/inputs/), default to "[platform_name].png" (platform_name from platform.ini)


 You can use multiple "homebrew" plugins each with its own romdir=. First six characters of magic number must be 484252. The last two can be random.


 EXAMPLES:
 =========

 /wiiflow/plugins/wii2600.ini may have these settings:

[PLUGIN]
displayname=Atari 2600
magic=32363030
dolfile=wii2600.dol
romdir=favoriteroms/atari2600
explorerpath=sd:/wii2600/roms
filetypes=.a26|.bin|.rom|.gz|.zip
coverfolder=a26
covercolor=000000
bannersound=2600.ogg
arguments={device}:/{path}/{name}


 /wiiflow/plugins/vbagx.ini may have those settings:

[PLUGIN]
displayname=Game Boy Advance
magic=56424188
dolfile=apps/vbagx/boot.dol
romdir=vbagx/roms
fileTypes=.agb|.gba|.bin|.elf|.mb|.dmg|.gb|.gbc|.cgb|.sgb|.zip|.7z
filenamesastitles=yes
coverfolder=gba
rompartition=1
boxmode=0
bannersound=GBA.ogg
arguments={device}:/{path}/|{name}|{loader}
