#include <ogc/system.h>

#include "defines.h"
#include "booter/external_booter.hpp"
#include "channel/nand.hpp"
#include "channel/nand_save.hpp"
#include "devicemounter/DeviceHandler.hpp"
#include "gecko/gecko.hpp"
#include "gui/video.hpp"
#include "homebrew/homebrew.h"
#include "loader/alt_ios_gen.h"
#include "loader/wdvd.h"
#include "loader/alt_ios.h"
#include "loader/cios.h"
#include "loader/nk.h"
#include "menu/menu.hpp"

bool isWiiVC = false;
bool useMainIOS = false; // use default IOS 58
bool sdOnly = false;
volatile bool NANDemuView = false;
volatile bool networkInit = false;
bool useMetaArgIOS = false; // IOS set as argument in meta.xml

int main(int argc, char **argv)
{
	MEM_init(); // Inits both mem1lo and mem2
	mainIOS = DOL_MAIN_IOS; // 249
	__exception_setreload(10);
	Gecko_Init(); // USB Gecko and SD/WiFi buffer
	gprintf(" \nWelcome to %s %s!\nThis is the debug output.\n", APP_NAME, APP_VERSION);
	
	bool iosOK = true;
	char *gameid = NULL;
	bool showFlashImg = true;
	bool wait_loop = false;
	char wait_dir[256];

	memset(&wait_dir, 0, sizeof(wait_dir));

	for(u8 i = 0; i < argc; i++)
	{
		if(argv[i] != NULL && strcasestr(argv[i], "ios=") != NULL && strlen(argv[i]) > 4)
		{
			while(argv[i][0] && !isdigit(argv[i][0]))
				argv[i]++;
			if(atoi(argv[i]) < 254 && atoi(argv[i]) > 0)
			{
				mainIOS = atoi(argv[i]);
				useMainIOS = mainIOS == 58 ? false : true;
				useMetaArgIOS = true;
			}
		}
		else if(strcasestr(argv[i], "waitdir=") != NULL)
		{
			char *ptr = strcasestr(argv[i], "waitdir=");
			strncpy(wait_dir, ptr+strlen("waitdir="), sizeof(wait_dir) - 1);
		}
		else if(strcasestr(argv[i], "Waitloop") != NULL)
			wait_loop = true;
		else if(strcasestr(argv[i], "noflash") != NULL)
			showFlashImg = false;
		else if(strlen(argv[i]) == 6)
		{
			gameid = argv[i];
			for(u8 i = 0; i < 5; i++)
			{
				if(!isalnum(gameid[i]))
					gameid = NULL;
			}
		}
	}

	/* Init video */
	m_vid.init();
	if(showFlashImg)
		m_vid.startImage();

	/* Check if WiiVC */
	WiiDRC_Init();
	isWiiVC = WiiDRC_Inited();

	if(IsOnWiiU())
	{	
		gprintf("WiiU");
		if(isWiiVC)	
			gprintf(" WiiVC\n");
		else
			gprintf(" vWii mode\n");
	}
	else
		gprintf("Real Wii\n");

	gprintf("AHBPROT disabled = %s\n", AHBPROT_Patched() ? "yes" : "no");

	/* Init device partition handlers */
	DeviceHandle.Init();
	
	/* Init NAND handlers */
	NandHandle.Init();

	if(isWiiVC)
	{
		NandHandle.Init_ISFS();
		IOS_GetCurrentIOSInfo();
		DeviceHandle.SetModes();
	}
	else
	{
		check_neek2o(); //
		NandHandle.Init_ISFS(); // Init ISFS
		if(InternalSave.CheckSave()) // load and check wiiflow save for possible new IOS and Port settings 
			InternalSave.LoadSettings(); // IOS will be retrieved from nand save only if there's no meta.xml IOS arg
		if(useMetaArgIOS)
		{
			gprintf("Loading IOS Settings from meta.xml\n");
			cur_load = mainIOS == 58 ? false : true;
			cur_ios = mainIOS == 58 ? 0 : mainIOS;
		}
		NandHandle.DeInit_ISFS();		
		
		NandHandle.Patch_AHB();
		if(useMainIOS && CustomIOS(IOS_GetType(mainIOS)))
		{
			iosOK = IOS_ReloadIOS(mainIOS) == 0; // load cIOS (249 by default)
			gprintf("Using IOS %d\n", mainIOS);
			gprintf("AHBPROT disabled after IOS Reload: %s\n", AHBPROT_Patched() ? "yes" : "no");
		}
		else
		{
			iosOK = IOS_ReloadIOS(58) == 0; // load IOS 58
			gprintf("Using IOS58\n");
		}
		NandHandle.Init_ISFS();
	}
	IOS_GetCurrentIOSInfo();
	if(CurrentIOS.Type == IOS_TYPE_HERMES)
		load_ehc_module_ex();
	else if(CurrentIOS.Type == IOS_TYPE_WANIN && CurrentIOS.Revision >= 18)
		load_dip_249();
	DeviceHandle.SetModes();
	
	/* Init DVD */
	WDVD_Init(); // needed for dvd checks
	
	/* Mount SD */
	DeviceHandle.MountSD();

	/* Mount USB if needed */
	bool usingUSB = (!sdOnly && !isWiiVC);
	if(usingUSB && showFlashImg)
		m_vid.usbImage(false); // splash usb not connected
	DeviceHandle.SetMountUSB(usingUSB);
	
	bool usb_mounted = false;
	int retry_count = 0;
	const int MAX_RETRIES = 5; // Maximum number of retries before asking user
	
	while(usingUSB && !usb_mounted && retry_count < MAX_RETRIES)
	{
		usb_mounted = DeviceHandle.MountAllUSB();
		if(!usb_mounted)
		{
			retry_count++;
			if(retry_count < MAX_RETRIES)
			{
				// Wait a bit before retrying
				usleep(1000000); // Wait 1 second between retries
				continue;
			}
		}
	}
	
	if(!usb_mounted)
	{
		DeviceHandle.SetMountUSB(false);
		// Power cycle the Wii instead of enabling SD-only mode
		gprintf("No USB device found after %d retries. Power cycling Wii...\n", MAX_RETRIES);
		ShutdownBeforeExit(); // unmount devices and close inputs
		Sys_Exit(); // This will power cycle the Wii
	}
	else if(showFlashImg)
		m_vid.usbImage(true); // splash usb connected
	
	/* Init wait images */
	m_vid.setCustomWaitImgs(wait_dir, wait_loop); // m_vid.waitMessage() is called later in mainMenu.init()

	/* Init controllers for input */
	Open_Inputs(); // WPAD_SetVRes() is called later in mainMenu.init() during cursor init which gets the theme pointer images

	/* Sys inits */
	Sys_Init(); // set reset and power button callbacks
	
	bool startup_successful = false;
	/* Init configs, folders, coverflow, gui and more */
	if(mainMenu.init(usb_mounted))
	{
		if(iosOK)
		{
			startup_successful = true;
			if(!isWiiVC)
				writeStub(); // copy return stub to memory
			if(!isWiiVC && gameid != NULL && strlen(gameid) == 6) // if argv game ID then launch it
				mainMenu.directlaunch(gameid);
			else
				mainMenu.main(); // start wiiflow with main menu displayed
		}
	}
	ShutdownBeforeExit(); // unmount devices and close inputs

	if(startup_successful) // use wiiflow's exit choice otherwise just exit to loader (system menu or hbc)
		Sys_Exit();
	return 0;
}
