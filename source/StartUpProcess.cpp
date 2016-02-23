#include <unistd.h>
#include <ogc/usb.h>
#include "StartUpProcess.h"
#include "GUI/gui.h"
#include "video.h"
#include "audio.h"
#include "input.h"
#include "themes/CTheme.h"
#include "debughelper/debughelper.h"
#include "Controls/DeviceHandler.hpp"
#include "wad/nandtitle.h"
#include "SystemMenu/SystemMenuResources.h"
#include "system/IosLoader.h"
#include "system/runtimeiospatch.h"
#include "utils/timer.h"
#include "settings/CSettings.h"
#include "settings/CGameSettings.h"
#include "settings/CGameStatistics.h"
#include "settings/CGameCategories.hpp"
#include "settings/GameTitles.h"
#include "usbloader/usb_new.h"
#include "usbloader/MountGamePartition.h"
#include "usbloader/GameBooter.hpp"
#include "usbloader/GameList.h"
#include "utils/tools.h"
#include "sys.h"
#include "svnrev.h"
#include <debug.h>

StartUpProcess::StartUpProcess()
{
	//! Load default font for the next text outputs
	Theme::LoadFont("");

	background = new GuiImage(screenwidth, screenheight, (GXColor) {0, 0, 0, 255});

	GXImageData = Resources::GetImageData("gxlogo.png");
	GXImage = new GuiImage(GXImageData);
	GXImage->SetAlignment(ALIGN_CENTER, ALIGN_MIDDLE);
	GXImage->SetPosition(screenwidth/2, screenheight/2-50);

	titleTxt = new GuiText("Loading...", 24, (GXColor) {255, 255, 255, 255});
	titleTxt->SetAlignment(ALIGN_CENTER, ALIGN_MIDDLE);
	titleTxt->SetPosition(screenwidth/2, screenheight/2+30);

	messageTxt = new GuiText(" ", 22, (GXColor) {255, 255, 255, 255});
	messageTxt->SetAlignment(ALIGN_CENTER, ALIGN_MIDDLE);
	messageTxt->SetPosition(screenwidth/2, screenheight/2+60);

	versionTxt = new GuiText(" ", 18, (GXColor) {255, 255, 255, 255});
	versionTxt->SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	versionTxt->SetPosition(20, screenheight-20);

	#ifdef FULLCHANNEL
	versionTxt->SetTextf("v3.0c Rev. %s", GetRev());
	#else
	versionTxt->SetTextf("v3.0  Rev. %s", GetRev());
	#endif

	#if 0 // enable if you release a modded version
	versionTxt->SetTextf("v3.0  Rev. %s mod", GetRev());
	#endif

	cancelTxt = new GuiText("Press B to cancel", 18, (GXColor) {255, 255, 255, 255});
	cancelTxt->SetAlignment(ALIGN_CENTER, ALIGN_MIDDLE);
	cancelTxt->SetPosition(screenwidth/2, screenheight/2+90);

	trigB = new GuiTrigger;
	trigB->SetButtonOnlyTrigger(-1, WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B, PAD_BUTTON_B);

	cancelBtn = new GuiButton(0, 0);
	cancelBtn->SetTrigger(trigB);

	drawCancel = false;
}

StartUpProcess::~StartUpProcess()
{
	delete background;
	delete GXImageData;
	delete GXImage;
	delete titleTxt;
	delete messageTxt;
	delete versionTxt;
	delete cancelTxt;
	delete cancelBtn;
	delete trigB;
}

int StartUpProcess::ParseArguments(int argc, char *argv[])
{
	int quickBoot = -1;

	//! The arguments override
	for(int i = 0; i < argc; ++i)
	{
		if(!argv[i]) continue;

		debughelper_printf("Boot argument %i: %s\n", i+1, argv[i]);

		char *ptr = strcasestr(argv[i], "-ios=");
		if(ptr)
		{
			if(atoi(ptr+strlen("-ios=")) == 58)
			Settings.LoaderIOS = 58;
			else
			Settings.LoaderIOS = LIMIT(atoi(ptr+strlen("-ios=")), 200, 255);
			Settings.UseArgumentIOS = ON;
		}

		ptr = strcasestr(argv[i], "-usbport=");
		if(ptr)
		{
			Settings.USBPort = LIMIT(atoi(ptr+strlen("-usbport=")), 0, 8);
		}

		ptr = strcasestr(argv[i], "-mountusb=");
		if(ptr)
		{
			Settings.USBAutoMount = LIMIT(atoi(ptr+strlen("-mountusb=")), 0, 1);
		}

		if(strlen(argv[i]) == 6 && strchr(argv[i], '=') == 0 && strchr(argv[i], '-') == 0)
		quickBoot = i;
	}

	return quickBoot;
}

void StartUpProcess::TextFade(int direction)
{
	if(direction > 0)
	{
		for(int i = 0; i < 255; i += direction)
		{
			messageTxt->SetAlpha(i);
			Draw();
		}
		messageTxt->SetAlpha(255);
		Draw();
	}
	else if(direction < 0)
	{
		for(int i = 255; i > 0; i += direction)
		{
			messageTxt->SetAlpha(i);
			Draw();
		}
		messageTxt->SetAlpha(0);
		Draw();
	}
}

void StartUpProcess::SetTextf(const char * format, ...)
{
	char * tmp = NULL;
	va_list va;
	va_start(va, format);
	if((vasprintf(&tmp, format, va) >= 0) && tmp)
	{
		TextFade(-40);
		messageTxt->SetText(tmp);
		debughelper_printf(tmp);
		TextFade(40);
	}
	va_end(va);

	if(tmp)
	free(tmp);
}

bool StartUpProcess::USBSpinUp()
{
	drawCancel = true;
	Timer countDown;

	bool started = false;

	// wait 20 sec for the USB to spin up...stupid slow ass HDD
	do
	{
		usbstorage_deinit();
		debughelper_printf("Iniciando USB");
		if(usbstorage_init())
			debughelper_printf("USB Iniciado com sucesso");
		else
			debughelper_printf("Erro ao iniciar USB");

		int numDevices = usbstorage_get_num_devices();

		debughelper_printf("Encontrados %d devices", numDevices);

		for(int i = 0; i < numDevices; i++){
			bool portStarted1 = DeviceHandler::Instance()->GetInterfaceUSB(i)->startup();
			bool portStarted2 = DeviceHandler::Instance()->GetInterfaceUSB(i)->isInserted();
			debughelper_printf("Porta %d iniciada com resultado %d %d", i, portStarted1, portStarted2);

			if(portStarted1 && portStarted2)
				started = true;
		}

		if(started)
			break;

		UpdatePads();
		for(int i = 0; i < 4; ++i)
			cancelBtn->Update(&userInput[i]);

		if(cancelBtn->GetState() == STATE_CLICKED)
			break;

		messageTxt->SetTextf("Waiting for HDD: %i sec left\n", 20-(int)countDown.elapsed());
		Draw();
		sleep(1);
	}
	while(countDown.elapsed() < 20.f);

	drawCancel = false;

	return started;
}

int StartUpProcess::Run(int argc, char *argv[])
{
	int quickGameBoot = ParseArguments(argc, argv);

	StartUpProcess Process;
	int ret = Process.Execute();

	if(quickGameBoot != -1)
	return QuickGameBoot(argv[quickGameBoot]);

	return ret;
}

int StartUpProcess::Execute()
{
	Settings.EntryIOS = IOS_GetVersion();

	if(IosLoader::LoadAppCios() < 0)
	{
		// We can allow now operation without cIOS in channel mode with AHB access
		if(!AHBPROT_DISABLED || (AHBPROT_DISABLED && IOS_GetVersion() != 58))
		{
			SetTextf("Failed loading IOS 58. USB Loader GX requires a cIOS or IOS 58 with AHB access. Exiting...\n");
			sleep(5);
			Sys_BackToLoader();
		}
		else
		{
			Settings.LoaderIOS = 58;
			SetTextf("Running on IOS 58. Wii disc based games and some channels will not work.");
			sleep(1);
		}
	}

	if(!AHBPROT_DISABLED && IOS_GetVersion() < 200)
	{
		SetTextf("Failed loading IOS %i. USB Loader GX requires IOS58 with AHB access. Exiting...\n", IOS_GetVersion());
		sleep(5);
		Sys_BackToLoader();
	}

	SetTextf("Using %sIOS %i\n", IOS_GetVersion() >= 200 ? "c" : "", IOS_GetVersion());
	SetupPads();

	SetTextf("Initialize sd card\n");
	DeviceHandler::Instance()->MountSD();

	if(Settings.USBAutoMount == ON)
	{
		SetTextf("Initialize usb device\n");
		USBSpinUp();
		DeviceHandler::Instance()->MountAllUSB();
		int count = DeviceHandler::Instance()->GetTotalPartitionCount();
		SetTextf("Total de %d partições\n", count);
	}

	SetTextf("Loading config files\n");

	debughelper_printf("\tLoading config...%s\n", Settings.Load() ? "done" : "failed");
	debughelper_printf("\tLoading language...%s\n", Settings.LoadLanguage(Settings.language_path, CONSOLE_DEFAULT) ? "done" : "failed");
	debughelper_printf("\tLoading game settings...%s\n", GameSettings.Load(Settings.ConfigPath) ? "done" : "failed");
	debughelper_printf("\tLoading game statistics...%s\n", GameStatistics.Load(Settings.ConfigPath) ? "done" : "failed");
	debughelper_printf("\tLoading game categories...%s\n", GameCategories.Load(Settings.ConfigPath) ? "done" : "failed");
	if(Settings.CacheTitles)
		debughelper_printf("\tLoading cached titles...%s\n", GameTitles.ReadCachedTitles(Settings.titlestxt_path) ? "done" : "failed (using default)");

	// enable isfs permission if using IOS+AHB or Hermes v4
	if(IOS_GetVersion() < 200 || (IosLoader::IsHermesIOS() && IOS_GetRevision() == 4))
	{
		SetTextf("Patching %sIOS%d...\n", IOS_GetVersion() >= 200 ? "c" : "", IOS_GetVersion());
		if (IosPatch_RUNTIME(true, false, false, false) == ERROR_PATCH)
			debughelper_printf("Patching %sIOS%d failed!\n", IOS_GetVersion() >= 200 ? "c" : "", IOS_GetVersion());
		else
			NandTitles.Get(); // get NAND channel's titles
	}

	// We only initialize once for the whole session
	ISFS_Initialize();

	// Check MIOS version
	SetTextf("Checking installed MIOS... ");
	IosLoader::GetMIOSInfo();

	SetTextf("Loading resources\n");
	// Do not allow banner grid mode without AHBPROT
	// this function does nothing if it was already initiated before
	if(!SystemMenuResources::Instance()->IsLoaded() && !SystemMenuResources::Instance()->Init()
	&& Settings.gameDisplay == BANNERGRID_MODE)
	{
		Settings.gameDisplay = LIST_MODE;
		Settings.GameWindowMode = GAMEWINDOW_DISC;
	}

	debughelper_printf("\tLoading font...%s\n", Theme::LoadFont(Settings.ConfigPath) ? "done" : "failed (using default)");
	debughelper_printf("\tLoading theme...%s\n", Theme::Load(Settings.theme) ? "done" : "failed (using default)");

	//! Init the rest of the System
	Sys_Init();
	InitAudio();
	setlocale(LC_CTYPE, "C-UTF-8");
	setlocale(LC_MESSAGES, "C-UTF-8");
	AdjustOverscan(Settings.AdjustOverscanX, Settings.AdjustOverscanY);

	return 0;
}

void StartUpProcess::Draw()
{
	background->Draw();
	GXImage->Draw();
	titleTxt->Draw();
	messageTxt->Draw();
	versionTxt->Draw();
	if(drawCancel)
	cancelTxt->Draw();
	Menu_Render();
}

int StartUpProcess::QuickGameBoot(const char * gameID)
{
	MountGamePartition(false);

	struct discHdr *header = NULL;
	for(int i = 0; i < gameList.size(); ++i)
	{
		if(strncasecmp((char *) gameList[i]->id, gameID, 6) == 0)
		header = gameList[i];
	}

	if(!header)
	return -1;

	GameStatistics.SetPlayCount(header->id, GameStatistics.GetPlayCount(header->id)+1);
	GameStatistics.Save();

	return GameBooter::BootGame(header);
}
