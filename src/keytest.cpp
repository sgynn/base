//This is a test thingy for outputting the keymap
#include <cstdio>
#include <string.h>
#include "base/input.h"

using namespace base;

void setTestKey(int keycode) {
	//Definition
	static unsigned char map[256];
	static const char* names[128] = {
		"KEY_A",	"KEY_B",	"KEY_C",	"KEY_D",	"KEY_E",	"KEY_F",	"KEY_G",	"KEY_H",	"KEY_I",	"KEY_J",
		"KEY_K",	"KEY_L",	"KEY_M",	"KEY_N",	"KEY_O",	"KEY_P",	"KEY_Q" ,	"KEY_R" ,	"KEY_S" ,	"KEY_T" ,
		"KEY_U" ,	"KEY_V" ,	"KEY_W" ,	"KEY_X" ,	"KEY_Y" ,	"KEY_Z" ,	"KEY_0" ,	"KEY_1" ,	"KEY_2" ,	"KEY_3" ,
		"KEY_4" ,	"KEY_5" ,	"KEY_6" ,	"KEY_7" ,	"KEY_8" ,	"KEY_9" ,	"KEY_0_PAD" ,"KEY_1_PAD" ,	"KEY_2_PAD" ,
		"KEY_3_PAD" ,	"KEY_4_PAD" ,	"KEY_5_PAD" ,	"KEY_6_PAD" ,	"KEY_7_PAD" ,	"KEY_8_PAD" ,
		"KEY_9_PAD" ,	"KEY_F1" ,	"KEY_F2" ,	"KEY_F3" ,	"KEY_F4" ,	"KEY_F5" ,
		"KEY_F6" ,	"KEY_F7" ,	"KEY_F8" ,	"KEY_F9" ,	"KEY_F10" ,	"KEY_F11" ,
		"KEY_F12" ,	"KEY_ESC" ,	"KEY_TICK" ,	"KEY_MINUS" ,	"KEY_EQUALS" ,	"KEY_BACKSPACE" ,
		"KEY_TAB" ,	"KEY_OPENBRACE" ,	"KEY_CLOSEBRACE" ,	"KEY_ENTER" ,	"KEY_COLON" ,	"KEY_QUOTE" ,
		"KEY_HASH" ,	"KEY_BACKSLASH" ,	"KEY_COMMA" ,	"KEY_STOP" ,	"KEY_SLASH" ,	"KEY_SPACE" ,
		"KEY_INSERT" ,	"KEY_DEL" ,	"KEY_HOME" ,	"KEY_END" ,	"KEY_PGUP" ,	"KEY_PGDN" ,	"KEY_LEFT" ,
		"KEY_RIGHT" ,	"KEY_UP" ,	"KEY_DOWN" ,	"KEY_SLASH_PAD" ,	"KEY_ASTERISK" ,	"KEY_MINUS_PAD" ,
		"KEY_PLUS_PAD" ,	"KEY_DEL_PAD" ,	"KEY_ENTER_PAD" ,	"KEY_PRTSCR" ,	"KEY_PAUSE" ,	"KEY_ABNT_C1" ,	"KEY_YEN" ,
		"KEY_KANA" ,	"KEY_CONVERT" ,	"KEY_NOCONVERT" ,	"KEY_AT" ,	"KEY_CIRCUMFLEX" ,	"KEY_COLON2" ,
		"KEY_KANJI" ,	"KEY_EQUALS_PAD" ,	"KEY_BACKQUOTE" ,	"KEY_SEMICOLON",	"KEY_COMMAND",	"KEY_UNKNOWN1",
		"KEY_UNKNOWN2",	"KEY_UNKNOWN3",	"KEY_UNKNOWN4",	"KEY_UNKNOWN5",	"KEY_UNKNOWN6",	"KEY_UNKNOWN7",	"KEY_UNKNOWN8",

		"KEY_LSHIFT",	"KEY_RSHIFT",	"KEY_LCONTROL",	"KEY_RCONTROL",	"KEY_ALT",	"KEY_ALTGR",	"KEY_LWIN",
		"KEY_RWIN",	"KEY_MENU",	"KEY_SCRLOCK",	"KEY_NUMLOCK",	"KEY_CAPSLOCK", "KEY_SPACE", "..."
	};
	static int last = -1;
	static char space = 0;

	//Processing
	if(last>=0) {
		map[keycode] = last;
		printf("%d\n", keycode);
	} else {
		memset(map, 0xff, 256);
		printf("Keyboard map creator - his will loop through all the keys\n");
		printf("Press SPACE for unknowns.\n\nPress any key to start.\n");
	}
	//Next key
	last++;
	if(last>=126) {
		if(last==126) printf("Done. Press K to restart\n");
		if(keycode==map[ KEY_K ]) last=-1;
		if(last==KEY_SPACE) space=keycode;
		last++;
		return;
	}
	printf("Enter %s: ", names[last]); fflush(stdout);

	// Output
	FILE* fp = fopen("keymap", "w");
	if(fp) {
		for(int i=0; i<256; i++) {
			if(map[i]==space && i!=KEY_SPACE) map[i]=0; //reset unknown to 0
			if(i%8==0) fprintf(fp, "\n/*0x%02x*/\t", i);
			if(map[i]==0xff) fprintf(fp,"%*d,", 14, 0);
			else fprintf(fp,"%*s,", 14, names[(int) map[i]] );
		}
		fclose(fp);
	}
}

