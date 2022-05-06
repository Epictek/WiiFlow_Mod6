
 Update April 29, 2022
 
 SOURCE MENU SETUP:
 ==================

 Source menu can be displayed in static pages style (classic source menu) or coverflow view (sourceflow).
 You can switch from classic to sourceflow view in Wiiflow settings > Source menu settings.

 Source menu can have multiple tiers defined in multiple ini files (see below), but the main page must be defined in "source_menu.ini".

 Each tier can have a different background image, and you can specify a different one for sourceflow menu and classic source menu, 
  PNG or JPG for classic menu source backgrounds must be placed in wiiflow/source_menu/backgrounds/
  PNG or JPG for sourceflow menu backgrounds must be placed in wiiflow/source_menu/backgrounds/sourceflow/

 Each classic source menu page can have 12 image buttons for each source.
 Each button is represented in the source_menu.ini as [BUTTON_#].
 
 Sourceflow attributes can be forced for each theme in wiiflow/themes_lite/[theme name].ini file by adding:

[_SOURCEFLOW]
box_mode=yes/no
enabled=yes/no
smallbox=yes/no
 

 BUTTONS LAYOUT IN CLASSIC SOURCE MENU:
 ======================================

 The buttons are positioned like this in classic source menu view:

 (Page 1)
0	1	2	3
4	5	6	7
8	9	10	11

 (Page 2)
12	13	14	15
16	17	18	19
20	21	22	23

 (Page 3)
24	25	26	27
28	29	30	31
32	33	34	35

 If you only have 4 buttons, they can be positioned in the middle of the screen if you only use buttons 4, 5, 6 and 7.
 

 FOLDER STRUCTURE:
 =================

 Each theme can have its own source menu, with different layouts, buttons, backgrounds, and small covers for sourceflow.
 Possibilities are as follows:

 1. The same source menu for all themes:
 ---------------------------------------

directory for ini files and classic menu button images:
 /wiiflow/source_menu/

directory for tier backgrounds (optional):
 /wiiflow/source_menu/backgrounds/

directory for sourceflow button images:
 /wiiflow/source_menu/small_covers/
 
 2. A unique layout for buttons in classic menu, but different images related to theme:
 --------------------------------------------------------------------------------------

directory for ini files:
 /wiiflow/source_menu/

directory for classic menu button images:
 /wiiflow/source_menu/[Theme name]/

directory for tier backgrounds (optional):
 /wiiflow/source_menu/backgrounds/[Theme name]/

directory for sourceflow button images:
 /wiiflow/source_menu/small_covers/[Theme name]/

 3. A different source menu for each theme:
 ------------------------------------------

directory for ini files and classic menu button images:
 /wiiflow/source_menu/[Theme name]/

directory for tier backgrounds (optional):
 /wiiflow/source_menu/[Theme name]/backgrounds/

directory for sourceflow button images:
 /wiiflow/source_menu/[Theme name]/small_covers/
 
 
NB: as an alternative to buttons, sourceflow can also display full box images, files must be placed in:
 /wiiflow/source_menu/full_covers/
Front covers only must be placed in:
 /wiiflow/source_menu/front_covers/
 

 SETTINGS FOR SOURCE MENU INI FILES:
 ===================================
 
[GENERAL]
pagetitle_1=		
# optional - title for page 1 - buttons 0 to 11 (classic source menu view only)

pagetitle_2=		
# optional - title for page 2 - buttons 12 to 23 (classic source menu view only)

background=		
# optional - background image (e.g. "atari.jpg") to use for the source menu tier (PNG or JPG must be placed in wiiflow/source_menu/backgrounds/)

flow=				
# optional - coverflow layout for sourceflow

[BUTTON_0]
title=				
# optional - title displayed under small cover (sourceflow view only)

source=			
# see BUTTON SOURCES section

magic=				
# plugin magic (MUST be specified if source=plugin, source=new_source or source=explorer)

image=				
# image for button (PNG must be placed in wiiflow/source_menu/ for classic source menu view and wiiflow/source_menu/small_covers/ for sourceflow view)

image_s=			
# image for selected button (pointer hover over, classic source menu view only - PNG must be placed in wiiflow/source_menu/)

cat_page=			
# optional - categories menu start page for easy access to categories related to the source

category=			
# optional - auto select this category (in "required" mode)

autoboot=			
# optional - game or homebrew app to boot directly with this button

hidden=
# optional - hide button (yes/no - default to no)

nandfolder=
# optional (only if source=emunand) - emunand folder ("device:/nands/[this_folder]")

background=		
# obsolete - background image associated with coverflow (backgrounds are now managed in plugins_data/platform.ini)

emuflow=			
# obsolete - coverflow style for plugins (all coverflow styles are now managed in plugins_data/platform.ini)


 BUTTON SOURCES:
 ===============

 There are 11 sources to choose from and they are as follows:

wii = Wii view
gamecube = Gamecube view ("dml" is obsoleted)
emunand = Emu NAND view
realnand = Real NAND view
bothnand = Emu + Real NAND
homebrew = Homebrew view (dol applications found in /apps directory)
allplugins = Plugin view (all plugins together)
plugin = Plugin view (a specific plugin specified by the magic number in "magic=" field, or multiple plugins separated by commas)
new_source = New tier of source menu specified by its file name (e.g. "handheld_consoles.ini") in "magic=" field
explorer = List view (file explorer style) for a specific plugin specified by the magic number in "magic=" field (not suitable for Wii channels)
back_tier = Go to previous tier (doesn't need any magic)


 EXAMPLES:
 =========

 This button will display the coverflow for Wii games, filtered on category 3:

[BUTTON_0]
title=Wii games
source=wii
image=wii.png
image_s=wii_s.png
category=3


 This button will autoboot the Mii Channel on real NAND:

[BUTTON_1]
title=Mii channel
source=realnand
autoboot=HACA
image=mii.png
image_s=mii_s.png


 This button will display the coverflow for Genesis games, that use GenPlusGX plugin, mixed with Atari 2600 games, that use Wii2600 plugin, with categories starting on page 2:

[BUTTON_2]
title=Genesis and Atari 2600 games
source=plugin
magic=53454761,32363030
image=gen_ata.png
image_s=gen_ata_s.png
cat_page=2


 This button (without any text under the small cover in sourceflow) will display a new tier of source menu, that contains all buttons for handheld consoles:

[BUTTON_3]
title=
source=new_source
magic=handheld.ini
image=handheld.png
image_s=handheld_s.png


 This button will display a file explorer (list view without coverflow) for NES games, that use FCEUGX plugin
 The path to explore must be defined in FCEUGX plugin ini file in "explorerpath=" field
 If path value is ommited, default will be "romdir" value

[BUTTON_4]
title=NES SmokeMonster Pack
source=explorer
magic=46434555
image=nes_pack.png
image_s=nes_pack_s.png

