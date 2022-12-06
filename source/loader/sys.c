
#include <ogc/lwp_threads.h>
#include <ogc/pad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "mload.h"
#include "sys.h"
#include "channel/channel_launcher.h"
#include "loader/nk.h"
#include "gecko/gecko.hpp"
#include "memory/mem2.hpp"
#include "memory/memory.h"
#include "sicksaxis-wrapper/sicksaxis-wrapper.h"
#include "wiiuse/wpad.h"
#include "wupc/wupc.h"
#include "defines.h" //

/* Variables */
u8 shutdown = 0; // modified
volatile u8 ExitOption = 0;
const char *NeekPath = NULL;

void __Wpad_PowerCallback()
{
	shutdown = POWEROFF_CONSOLE;
}

void Open_Inputs(void)
{
	/* Initialize controllers - wiiu drc is inited only once in main.cpp */
	PAD_Init();
	WUPC_Init();
	WPAD_Init();
	DS3_Init();
	
	/* Set wiimote power button callback */
	WPAD_SetPowerButtonCallback(__Wpad_PowerCallback);
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetIdleTimeout(IDLE_TIME); // IDLE_TIME in defines.h
}

void Close_Inputs(void)
{
	WUPC_Shutdown();

	/* Disconnect Wiimotes */
	for(u32 cnt = 0; cnt < 4; cnt++)
		WPAD_Disconnect(cnt);

	/* Shutdown Wiimote subsystem */
	WPAD_Shutdown();
	
	DS3_Cleanup();
}

u8 Sys_Exiting(void) // modified
{
	DCFlushRange(&shutdown, 32);
	return shutdown;
}

int Sys_GetExitTo(void)
{
	return ExitOption;
}

void Sys_ExitTo(int option)
{
	ExitOption = option;
	// magic word to force wii menu in priiloader.
	if(ExitOption == EXIT_TO_MENU)
	{
		*Priiloader_CFG1 = 0x50756E65; // Pune
		*Priiloader_CFG2 = 0x50756E65;
	}
	else if(ExitOption == EXIT_TO_PRIILOADER)
	{
		*Priiloader_CFG1 = 0x4461636F; // Daco
		*Priiloader_CFG2 = 0x4461636F;
	}
	else // PRIILOADER_DEF
	{
		*Priiloader_CFG1 = 0xFFFFFFFF;
		*Priiloader_CFG2 = 0xFFFFFFFF;
	}
	DCFlushRange((void*)Priiloader_CFG1, 4);
	DCFlushRange((void*)Priiloader_CFG2, 4);
}

void Sys_Exit(void)
{
	/* Shutdown Inputs */
	Close_Inputs();

	/* Poweroff console (standby or idle depending ow wc24 status) */
	if(ExitOption == POWEROFF_CONSOLE)
		SYS_ResetSystem(SYS_POWEROFF, 0, 0);
	/* Reboot console */
	if(ExitOption == RESET_CONSOLE)
		SYS_ResetSystem(SYS_RESTART, 0, 0);
	/* Force standby console */
	if(ExitOption == SHUTDOWN_STANDBY)
		SYS_ResetSystem(SYS_POWEROFF_STANDBY, 0, 0);	

	if(!isWiiVC)
	{
		/* We wanna boot something */
		WII_Initialize();
		/* If in neek2o mode Launch_nk will just return to neek2o system menu and not launch anything */
		if(ExitOption == EXIT_TO_WFNK2O)
#ifdef APP_WIIFLOW_LITE
			Launch_nk(0x1000157464C41LL, NeekPath, 0); // 57464C41 = WFLA
#else
			Launch_nk(0x1000144574641LL, NeekPath, 0); // 44574641 = DWFA
#endif	
		else if(ExitOption == EXIT_TO_SMNK2O)
			Launch_nk(0, NeekPath, 0);
		else if(ExitOption == EXIT_TO_BOOTMII)
			IOS_ReloadIOS(0xfe); // IOS254 Bootmii IOS
		else if(ExitOption == EXIT_TO_HBC)
		{
			WII_LaunchTitle(HBC_OHBC);
			WII_LaunchTitle(HBC_LULZ);
			WII_LaunchTitle(HBC_108);
			WII_LaunchTitle(HBC_JODI);
			WII_LaunchTitle(HBC_HAXX);
		}
		else if(ExitOption == EXIT_TO_WIIU)
			WII_LaunchTitle(WIIU_CHANNEL);
		else 	
		/* else Return to System Menu */
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	}
	exit(0);
}

void __Sys_ResetCallback(u32 irq, void *ctx)
{
	(void)irq; // to remove unused parameter warning
	(void)ctx;
	shutdown = RESET_CONSOLE;
}

void __Sys_PowerCallback(void)
{
	shutdown = POWEROFF_CONSOLE;
}

void Sys_Init(void)
{
	/* Set RESET/POWER button callback */
	if(!isWiiVC)
		SYS_SetResetCallback(__Sys_ResetCallback);
	SYS_SetPowerCallback(__Sys_PowerCallback);
}

bool AHBPROT_Patched(void)
{
	return (*HW_AHBPROT == 0xFFFFFFFF);
}

/** WiiU Check by crediar, thanks
This check doesn't work in neek2o mode or if WF boot.dol is launched from WiiXplorer **/
bool IsOnWiiU(void)
{
	return ((*HW_PROCESSOR >> 16 ) == 0xCAFE);
}

void Sys_SetNeekPath(const char *Path)
{
	NeekPath = Path;
}
/*
bool hw_checked = false;
bool on_hw = false;
bool Sys_HW_Access(void)
{
	if(hw_checked == true)
		return on_hw;

	on_hw = AHBPROT_Patched() && !neek2o();
	hw_checked = true;
	return on_hw;
}
*/
/* KILL IT */
s32 __IOS_LoadStartupIOS() { return 0; }


