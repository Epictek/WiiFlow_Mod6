# WiiFlow Mod6
My mod of the Wii USB Loader WiiFlow

## Description
I've been working on this fork of WiiFlow for quite a long time for my own personal use and decided I should eventually release it.
It brings back some features of WiiFlow 4 and many new features and quality of life changes.

## Changes
- Added ability to jump to a specific game using a virtual keyboard on screen.
- Install game feature is back, with the help of USB Loader GX source code.
- Added ability to install wad folders (batch mode).
- FTP server is back.
- Icons and menus have been completely rewritten and redrawn to have a more consistent look, settings are now categorized and themes are easier to write and maintain.
- Almost all settings can now be tweaked within WiiFlow menus, no need to edit ini files anymore.
- Neek2o mode is back: WiiFlow can be relaunched under neek2o for channel coverflow only.
- Added many checks and error messages when it comes to use neek2o features, with the help of USB Loader GX source code.
- Game categories can now be automatically retrieved from GameTDB and Wiimpathy's databases.
- If no categories ini file is found, a set of 20 basic categories (Action, platformer, racing...) is created.
- Added ability to create new custom categories within WiiFlow interface.
- Added ability to translate categories in language ini file.
- Added ability to launch an explorer view within source menu and coverflow config menu (text list view of games without coverflow), all games including plugins can be launched from there.
- The game folder browsed can be different from the one used by coverflow, and you can "import" games from this other folder to the coverflow, including CD based games (folder with BIN/CUE files).
- CD based games (folder with BIN/CUE files) can now be deleted.
- Added other features to source menu (listed in source_menu_readme.txt).
- Improved the process of caching covers to simplify the use of boxcover packs for plugin games (the image file simply called "Another World.png" will be automatically applied to the game "Another World (1991)(Delphine)(Disk 1 of 2)[cr Elite].st" if found).
- Added ability to display a game input guide for plugins in game menu.
- Added other features to plugins (listed in plugins_readme.txt).
- Added ability to create multiple emunand folders within WiiFlow interface.
- You can quickly switch to a specific emunand using B+left / B+right.
- Emunand folder can also be declared in source_menu.ini file for emunand source.
- Inputs have been rewritten so that Wiimote can be used either in vertical mode or sideways (A is 2, B is 1).
- All default images and useful binary files are now embedded in boot.dol, no more bins and images folder.
- Added ability to backup / delete / restore a game save on emunand or a GC virtual MemCard.
- Added 1019 blocks Nintendont virtual MemCard option.
- Video trailers can be zoomed and unzoomed just like game banners.
- Rewrite the way IOS settings can be forced using meta.xml arguments, so that it actually bypasses wiiflow nand save settings.
- IOS 58 is now default, change it to 250 (or whatever slot you use for cIOS) if you mainly launch Wii games and your hard drive supports it well.
- Game info displays playcount and last played info.
- Game menu warns you with specific icons if cheats or actual GC MemCard are activated.
- Improved dump game list feature, so that the file created can easily be used as a custom title file.
- Fixed an issue that prevented games with no banner title to be listed in coverflow.
- Simplified the cover download from GameTDB feature (with an option to force US region even for PAL games).
- Added option to retrieve disc label images from GameTDB (shown in game info).
- Rumble is attenuated.
- Removed white progress bar when launching games.
- Created a new theme called melodii, and adapted rhapsodii, carbonik and whitewii themes to make them work with this fork.
- Added ability to force source menu attributes (sourceflow, smallbox, boxmode) in theme ini files.
- Heavily modified wording of GUI, currently only english and french languages are available.
- Modified handling of fanart, animation is delayed and acts as a screen saver.
- Menus and settings are adapting to context (Wii, vWii, WiiU VC, neek...).
- Fixed many minor bugs and made hundreds of other minor changes that would be too long to list.
- Removed Devolution support.

## Screenshots
![Keyboard](https://github.com/iamerror80/WiiFlow_Mod6/blob/master/wii/wiiflow/screenshots/keyboard.png?raw=true)
![Categories](https://github.com/iamerror80/WiiFlow_Mod6/blob/master/wii/wiiflow/screenshots/categories.png?raw=true)
![Settings](https://github.com/iamerror80/WiiFlow_Mod6/blob/master/wii/wiiflow/screenshots/settings.png?raw=true)
![Cheats](https://github.com/iamerror80/WiiFlow_Mod6/blob/master/wii/wiiflow/screenshots/cheats.png?raw=true)
![Game info](https://github.com/iamerror80/WiiFlow_Mod6/blob/master/wii/wiiflow/screenshots/gameinfo.png?raw=true)
![Input guide](https://github.com/iamerror80/WiiFlow_Mod6/blob/master/wii/wiiflow/screenshots/inputguide.png?raw=true)

## Booting
To start WiiFlow Mod6 you will need the Homebrew Channel or a WiiFlow forwarder channel installed on your Wii or vWii system menu.
