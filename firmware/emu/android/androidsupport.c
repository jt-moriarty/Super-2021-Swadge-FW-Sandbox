//Copyright (c) 2011-2020 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.
// NO WARRANTY! NO GUARANTEE OF SUPPORT! USE AT YOUR OWN RISK

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "CNFG.h"
#include "os_generic.h"
#include "CNFG3D.h"
#include <GLES3/gl3.h>
#include <asset_manager.h>
#include <asset_manager_jni.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/sensor.h>

#include "../sound/sound.h"

#include "../../user/user_main.h"
#include "../../user/hdw/QMA6981.h"
#include "../../user/hdw/buzzer.h"
#include "../../user/hdw/buttons.h"
#include "spi_flash.h"

#include "../swadgemu.h"

#define LOGI(...)  ((void)__android_log_print(ANDROID_LOG_INFO, APPNAME, __VA_ARGS__))
#define printf( x...) LOGI( x )

//Actually from rawdraw
extern int UpdateScreenWithBitmapOffsetX;
extern int UpdateScreenWithBitmapOffsetY;


ASensorManager * sm;
const ASensor * as;
ASensorEventQueue* aeq;
ALooper * l;


void SetupIMU()
{
	sm = ASensorManager_getInstance();
	printf( "SM: %p\n", sm );
	as = ASensorManager_getDefaultSensor( sm, ASENSOR_TYPE_ACCELEROMETER);
	printf( "AS: %p\n", as );
	l = ALooper_prepare( ALOOPER_PREPARE_ALLOW_NON_CALLBACKS );
	printf( "L: %p\n", l );
	aeq = ASensorManager_createEventQueue( sm, (ALooper*)&l, 0, 0, 0 ); //XXX??!?! This looks wrong.
	printf( "AEQ: %p\n", aeq );
	ASensorEventQueue_enableSensor( aeq, as);
	printf( "setEvent Rate: %d\n", ASensorEventQueue_setEventRate( aeq, as, 10000 ) );
}

float accx, accy, accz;
int accs;

void AccCheck()
{
	ASensorEvent evt;
	if( aeq == 0 ) return; //If not configured, don't try.

	do
	{
		ssize_t s = ASensorEventQueue_getEvents( aeq, &evt, 1 );
		if( s <= 0 ) 
		{
			break;
		}
		if(ASENSOR_TYPE_ACCELEROMETER == evt.type)
		{
			accx = evt.acceleration.x;
			accy = evt.acceleration.y;
			accz = evt.acceleration.z;
		}
		accs++;
	} while( 1 );
	// printf("%s (%-2.4f, %-2.4f, %-2.4f)", __func__, accx, accy, accz);
}

unsigned frames = 0;
unsigned long iframeno = 0;

void AndroidDisplayKeyboard(int pShow);

int lastbuttonx = 0;
int lastbuttony = 0;
int lastmotionx = 0;
int lastmotiony = 0;
int lastbid = 0;
int lastmask = 0;
int lastkey, lastkeydown;

static int keyboard_up;

void HandleKey( int keycode, int bDown )
{
	lastkey = keycode;
	lastkeydown = bDown;
//	if( keycode == 10 && !bDown ) { keyboard_up = 0; AndroidDisplayKeyboard( keyboard_up );  }
}

extern int debuga, debugb, debugc;

void HandleButton( int x, int y, int button, int bDown )
{
	lastbid = button;
	lastbuttonx = x - UpdateScreenWithBitmapOffsetX;
	lastbuttony = y - UpdateScreenWithBitmapOffsetY;
	emuCheckFooterMouse( x, y, button, bDown );
}

//On android i is button number, not mask.
void HandleMotion( int x, int y, int i )
{
	lastmask = i;
	lastmotionx = x - UpdateScreenWithBitmapOffsetX;
	lastmotiony = y - UpdateScreenWithBitmapOffsetY;
	emuCheckFooterMouse( x, y, i, 1 );
}

#define HMX 162
#define HMY 162
short screenx, screeny;

extern struct android_app * gapp;

void HandleDestroy()
{
	printf( "Destroying\n" );
	exit(10);
}

volatile int suspended;

void HandleSuspend()
{
	suspended = 1;
}

void HandleResume()
{
	suspended = 0;
}


void QMA6981_poll(accel_t* currentAccel)
{
	// Convert Android's sensor readings into the Swadge's coordinates and scale
	currentAccel->x = accy * 256 / 9.81f;
	currentAccel->y = accx * 256 / 9.81f * -1;
	currentAccel->z = accz * 256 / 9.81f;
	// printf("%s (%5d, %5d, %5d)", __func__, currentAccel->x, currentAccel->y, currentAccel->z);
}

bool QMA6981_setup(void)
{
	SetupIMU();
	return true;
}


uint32_t randomtexturedata[256*256];

int main()
{
	int i, x, y;
	double ThisTime;
	double LastFPSTime = OGGetAbsoluteTime();
	double LastFrameTime = OGGetAbsoluteTime();
	double SecToWait;
	int linesegs = 0;

	//Tricky: on Android, this function doesn't automatically run before main.
	REGISTERAndroidSound();

	SoundInitFn * SoundDrivers[MAX_SOUND_DRIVERS];
	printf( "SOUNDDRIVER: %p\n", SoundDrivers[0] );


	CNFGBGColor = 0x000000;
	CNFGDialogColor = 0x444444;
	CNFGSetupFullscreen( "Test Bench", 0 );
	rawvidmem = malloc( px_scale * OLED_WIDTH*px_scale * (HEADER_PIXELS + OLED_HEIGHT+FOOTER_PIXELS)*px_scale * 4 );


	const char * assettext = "Not Found";
	AAsset * file = AAssetManager_open( gapp->activity->assetManager, "asset.txt", AASSET_MODE_BUFFER );
	if( file )
	{
		size_t fileLength = AAsset_getLength(file);
		char * temp = malloc( fileLength + 1);
		memcpy( temp, AAsset_getBuffer( file ), fileLength );
		temp[fileLength] = 0;
		assettext = temp;
	}

	boottime = OGGetAbsoluteTime();

	initOLED(0);

	void user_init();
	user_init();

	while(1)
	{
		int i, pos;
		float f;

		system_os_check_tasks();
		ets_timer_check_timers();

		updateOLED(0);

		CNFGHandleInput();
		AccCheck();

		CNFGClearFrame();
		CNFGColor( 0xFFFFFF );
		emuCheckResize();

		emuHeader();
		emuFooter();

		UpdateScreenWithBitmapOffsetY = 50;
		UpdateScreenWithBitmapOffsetX = (screenx - OLED_WIDTH*px_scale)/2;
		CNFGUpdateScreenWithBitmap( rawvidmem, OLED_WIDTH*px_scale, (HEADER_PIXELS + OLED_HEIGHT+FOOTER_PIXELS)*px_scale  );

		frames++;
		CNFGSwapBuffers();

		ThisTime = OGGetAbsoluteTime();
		if( ThisTime > LastFPSTime + 1 )
		{
			printf( "FPS: %d\n", frames );
			frames = 0;
			linesegs = 0;
			LastFPSTime+=1;
		}

		SecToWait = .01 - ( ThisTime - LastFrameTime );
		LastFrameTime += .01;
		if( SecToWait > 0 )
			OGUSleep( (int)( SecToWait * 1000000 ) );
	}

	return(0);
}

