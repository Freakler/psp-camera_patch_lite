#include <pspsdk.h> 
#include <pspkernel.h> 
#include <pspctrl.h> 
#include <stdio.h> 
#include <string.h> 
#include <systemctrl.h> 

#include "minini/minIni.h"

PSP_MODULE_INFO("camera_patch_lite", 0x1007, 1, 0);  // 0x1007

#define THRESHOLD 32 

#define PSP_CTRL_LTRIGGER 	0x000100
#define PSP_CTRL_RTRIGGER 	0x000200
#define PSP_CTRL_TRIANGLE 	0x001000
#define PSP_CTRL_CIRCLE 	0x002000
#define PSP_CTRL_CROSS 		0x004000
#define PSP_CTRL_SQUARE 	0x008000 
#define PSP_CTRL_LEFT 		0x000080 
#define PSP_CTRL_RIGHT 		0x000020 
#define PSP_CTRL_UP 		0x000010 
#define PSP_CTRL_DOWN 		0x000040 
#define PSP_CTRL_SELECT		0x000001 
#define PSP_CTRL_START		0x000008 

#define Rx Rsrv[0] //for readability
#define Ry Rsrv[1]

#define MAKE_JUMP(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a); 

#define HIJACK_FUNCTION(a, f, ptr) { \
	u32 func = a; \
	static u32 patch_buffer[3]; \
	_sw(_lw(func), (u32)patch_buffer); \
	_sw(_lw(func + 4), (u32)patch_buffer + 8);\
	MAKE_JUMP((u32)patch_buffer + 4, func + 8); \
	_sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), func); \
	_sw(0, func + 4); \
	ptr = (void *)patch_buffer; \
} 

int time = 0;
int delay = 0;
int contr = 0;

char gameid[16];

char config[256], *ptr;
const char *config_name = "camera_patch_lite.ini";

int BTN_TRG_1;
int BTN_TRG_2;
int BTN_CTRL_LEFT;
int BTN_CTRL_RIGHT;
int BTN_CTRL_UP;
int BTN_CTRL_DOWN;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int doesFileExist(const char* path) { 
	SceUID dir = sceIoOpen(path, PSP_O_RDONLY, 0777); 
 	if (dir >= 0) { 
 		sceIoClose(dir); 
 		return 1; 
 	} else { 
 		return 0; 
 	} 
} 

int check_adrenaline() {
	return doesFileExist("flash1:/config.adrenaline");
}

int try_get_game_info() {
	SceUID fd;
	do {
		fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777); 
		sceKernelDelayThread(1000);
	} while( fd <= 0 );
	sceIoRead(fd, gameid, 10);	
	sceIoClose(fd);
    return 1;
}

int getbutton(const char* input) { 

	if( strcmp(input, "LTRIGGER") == 0 ) {
		return PSP_CTRL_LTRIGGER;
	 
	} else if( strcmp(input, "RTRIGGER") == 0 ) {
		return PSP_CTRL_RTRIGGER;
		
	} else if( strcmp(input, "SQUARE") == 0 ) {
		return PSP_CTRL_SQUARE;
		
	} else if( strcmp(input, "CROSS") == 0 ) {
		return PSP_CTRL_CROSS;
		
	} else if( strcmp(input, "CIRCLE") == 0 ) {
		return PSP_CTRL_CIRCLE;
		
	} else if( strcmp(input, "TRIANGLE") == 0 ) {
		return PSP_CTRL_TRIANGLE;
		
	} else if( strcmp(input, "LEFT") == 0 ) {
		return PSP_CTRL_LEFT;
		
	} else if( strcmp(input, "RIGHT") == 0 ) {
		return PSP_CTRL_RIGHT;
		
	} else if( strcmp(input, "UP") == 0 ) {
		return PSP_CTRL_UP;
		
	} else if( strcmp(input, "DOWN") == 0 ) {
		return PSP_CTRL_DOWN;
		
		
	} else if( strcmp(input, "ANALOG_UP") == 0 ) {
		return -1;
		
	} else if( strcmp(input, "ANALOG_DOWN") == 0 ) {
		return -2;
		
	} else if( strcmp(input, "ANALOG_LEFT") == 0 ) {
		return -3;
		
	} else if( strcmp(input, "ANALOG_RIGHT") == 0 ) {
		return -4;
		
	} else return 0;
}


int check_ini() {	

	static char buff[128];
	int use;

		///check if category with TitleID exists and if it is enabled
		use = ini_getbool(gameid, "enable", 0, config);
		
		if ( use ) {
						
			///trigger button #1
			ini_gets(gameid, "trigger_button_1", "0", buff, sizeof(buff), config);
			BTN_TRG_1 = getbutton(buff);
			memset(buff,0,sizeof(buff)); //clear
			
			///trigger button #2
			ini_gets(gameid, "trigger_button_2", "0", buff, sizeof(buff), config);
			BTN_TRG_2 = getbutton(buff);
			memset(buff,0,sizeof(buff)); //clear
			
			///delay time
			delay = ini_getl(gameid, "camera_delay", 0, config);
			
			///camera movement button left
			ini_gets(gameid, "camera_left", "0", buff, sizeof(buff), config);
			BTN_CTRL_LEFT = getbutton(buff);
			memset(buff,0,sizeof(buff)); //clear
			
			///camera movement button right
			ini_gets(gameid, "camera_right", "0", buff, sizeof(buff), config);
			BTN_CTRL_RIGHT = getbutton(buff);
			memset(buff,0,sizeof(buff)); //clear
			
			///camera movement button up
			ini_gets(gameid, "camera_up", "0", buff, sizeof(buff), config);
			BTN_CTRL_UP = getbutton(buff);
			memset(buff,0,sizeof(buff)); //clear
			
			///camera movement button down
			ini_gets(gameid, "camera_down", "0", buff, sizeof(buff), config);
			BTN_CTRL_DOWN = getbutton(buff);
			memset(buff,0,sizeof(buff)); //clear
			
			
			return 1;
		}		
		
	return 0;
}


int (* sceCtrlReadBuf)(SceCtrlData *pad, int nBufs, int a2, int mode);

int sceCtrlReadBufPatched(SceCtrlData *pad, int nBufs, int a2, int mode) { 
		
	/// A static buffer 
	static char buffer[64 * 48];
	
	/// Set k1 to zero to allow all buttons 
	int k1 = pspSdkSetK1(0);

	/// Read PSP hardware buttons 
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	
	/// Call function with new arguments 
	int res = sceCtrlReadBuf((SceCtrlData *)buffer, nBufs, a2, mode); 
	//int res = sceCtrlReadBuf((SceCtrlData *)buffer, nBufs, 1, mode | 4);	//extended mode
	
	/// Copy buffer to pad
	int i; 
	for(i = 0; i < nBufs; i++) {
		memcpy(&pad[i], buffer + (i * 48), 12); 		
			
		//analog is unused (this is for delay handling)
		if((pad[i].Rx <= 127 + THRESHOLD) && (pad[i].Rx > 127 - THRESHOLD) && (pad[i].Ry <= 127 + THRESHOLD) && (pad[i].Ry > 127 - THRESHOLD)) { 
			time = sceKernelGetSystemTimeLow();
		}
		
		//analog is used
		if((pad[i].Rx >= 127 + THRESHOLD) | (pad[i].Rx < 127 - THRESHOLD) | (pad[i].Ry >= 127 + THRESHOLD) | (pad[i].Ry < 127 - THRESHOLD)) { 
			pad[i].Buttons |= BTN_TRG_1; //simulate camera button 1 pressed
			pad[i].Buttons |= BTN_TRG_2; //simulate camera button 2 pressed
						
			if( sceKernelGetSystemTimeLow() >= time + delay ) { //wait delay

					
				///camera right
				if( BTN_CTRL_RIGHT > 0 ) { //its a button
					if( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Buttons |= BTN_CTRL_RIGHT;
					
				} else if( BTN_CTRL_RIGHT < 0 ) { //its the analog
					switch(BTN_CTRL_RIGHT) {
						case -1: if( pad[i].Ry <= 127 - THRESHOLD ) pad[i].Lx = 255-pad[i].Ry; break; //analog up
						case -2: if( pad[i].Ry >= 127 + THRESHOLD ) pad[i].Lx = pad[i].Ry; break; //analog down
						case -3: if( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Lx = 255-pad[i].Rx; break; //analog left
						case -4: if( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Lx = pad[i].Rx; break; //analog right
						default: break;
					}
				}
				
				///camera left
				if( BTN_CTRL_LEFT > 0 ) { //its a button
						if( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Buttons |= BTN_CTRL_LEFT;
						
				} else if( BTN_CTRL_LEFT < 0 ) { //its the analog
					switch(BTN_CTRL_LEFT) {
						case -1: if( pad[i].Ry <= 127 - THRESHOLD ) pad[i].Lx = pad[i].Ry; break; //analog up
						case -2: if( pad[i].Ry >= 127 + THRESHOLD ) pad[i].Lx = 255-pad[i].Ry; break; //analog down
						case -3: if( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Lx = pad[i].Rx; break; //analog left
						case -4: if( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Lx = 255-pad[i].Rx; break; //analog right
						default: break;
					}
				}
					
				///camera up
				if( BTN_CTRL_UP > 0 ) { //its a button
					if( pad[i].Ry <= 127 - THRESHOLD ) pad[i].Buttons |= BTN_CTRL_UP;
						
				} else if( BTN_CTRL_UP < 0 ) { //its the analog
					switch(BTN_CTRL_UP) {
						case -1: if( pad[i].Ry <= 127 - THRESHOLD ) pad[i].Ly = pad[i].Ry; break; //analog up
						case -2: if( pad[i].Ry >= 127 + THRESHOLD ) pad[i].Ly = 255-pad[i].Ry; break; //analog down
						case -3: if( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Ly = pad[i].Rx; break; //analog left
						case -4: if( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Ly = 255-pad[i].Rx; break; //analog right
						default: break;
					}
				}
				
				///camera down
				if( BTN_CTRL_DOWN > 0 ) { //its a button
					if( pad[i].Ry >= 127 + THRESHOLD ) pad[i].Buttons |= BTN_CTRL_DOWN;
						
				} else if( BTN_CTRL_DOWN < 0 ) { //its the analog
					switch(BTN_CTRL_DOWN) {
						case -1: if( pad[i].Ry <= 127 - THRESHOLD ) pad[i].Ly = 255-pad[i].Ry; break; //analog up
						case -2: if( pad[i].Ry >= 127 + THRESHOLD ) pad[i].Ly = pad[i].Ry; break; //analog down
						case -3: if( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Ly = 255-pad[i].Rx; break; //analog left
						case -4: if( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Ly = pad[i].Rx; break; //analog right
						default: break;
					}
				}
				
					
			}
		}
		
		pad[i].Ry = 127;
		pad[i].Rx = 127;
	} 

	pspSdkSetK1(k1); //restore k1
	return res; //return result 
} 


int camera_thread(SceSize args, void *argp) {
	
	sceKernelDelayThread(3000000); //delay 3 sec for game to load
	
	while(!sceKernelFindModuleByName("sceKernelLibrary")) //wait for the kernel to boot up
		sceKernelDelayThread(100000);
	
	try_get_game_info(); //read game id from umd file
	
		
	if ( check_ini() ) { //check if game(id) supported & enabled
		
		SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceController_Service"); //Find ctrl.prx
			
		int i; ///Find function and hook it
		for(i = 0; i < mod->text_size; i += 4) { 
			u32 addr = mod->text_addr + i; 
				
			if(_lw(addr) == 0x35030104) {
				HIJACK_FUNCTION(addr - 0x1C, sceCtrlReadBufPatched, sceCtrlReadBuf); 
				break; 
			} 
		} 
		
		///Clear caches
		sceKernelDcacheWritebackAll(); 
		sceKernelIcacheClearAll(); 
	}	
	return 0;
}

	
int module_start(SceSize args, void *argp) {
	
	//ini location depending on where prx is 
	strcpy(config, (char*)argp);
	ptr = strrchr(config, '/');
	if(!ptr) return -1; //error
	strcpy(ptr + 1, config_name);

	//check for adrenline ecfw
	if( check_adrenaline() ) { 
		
		//create thread
		SceUID thid = sceKernelCreateThread("camera_thread", camera_thread, 0x18, 0x1000, 0, NULL);
		if(thid >= 0) sceKernelStartThread(thid, args, argp);
	
		sceKernelExitDeleteThread(0);
	}
	
	return 0;
}



