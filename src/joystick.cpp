#include <base/input.h>
#include <cstdio>


using namespace base;


bool Joystick::button(uint i) const {
	return m_buttons & (1<<i);
}
bool Joystick::pressed(uint i) const {
	return m_buttons & m_changed & (1<<i);
}
bool Joystick::released(uint i) const {
	return ~m_buttons & m_changed & (1<<i);
}
float Joystick::axis(uint i) const {
	if(i>=m_numAxes) return 0;
	
	int h = (m_range[i].max - m_range[i].min) / 2;
	h -= m_dead * h;

	int a = m_range[i].max - h;
	if(m_axis[i] > a) return (m_axis[i] - a) / (float) h;

	int b = m_range[i].min + h;
	if(m_axis[i] < b) return (m_axis[i] - b) / (float) h;

	return 0;
}
int Joystick::axisRaw(uint i) const {
	if(i>=m_numAxes) return 0;
	return m_axis[i];
}
const Point& Joystick::hat() const {
	return m_hat;
}

void Joystick::setDeadzone(float dead) {
	m_dead = dead;
}
void Joystick::getCalibration(uint n, int* data) const {
	if(n>=m_numAxes) n = m_numAxes;
	for(uint i=0; i<n; ++i) {
		data[i*2] = m_range[i].min;
		data[i*2+1] = m_range[i].max;
	}
}
void Joystick::setCalibration(uint n, const int* data) {
	if(n>=m_numAxes) n = m_numAxes;
	for(uint i=0; i<n; ++i) {
		m_range[i].min = data[i*2];
		m_range[i].max = data[i*2+1];
	}
}

// ================================================================= //

void Input::updateJoysticks() {
	for(Joystick* j: m_joysticks) if(j) j->update();
}
Joystick& Input::joystick(uint i) const {
	if(i<m_joysticks.size() && m_joysticks[i]) return *m_joysticks[i];
	static Joystick dummy(0,0);
	return dummy;
}

int Input::addJoystick(Joystick* j, int forceId) {
	if(forceId < 0) {
		j->m_index = m_joysticks.size();
		m_joysticks.push_back(j);
	}
	else {
		while(forceId >= (int)m_joysticks.size()) m_joysticks.push_back(nullptr);
		if(m_joysticks[forceId]) delete m_joysticks[forceId];
		j->m_index = forceId;
		m_joysticks[forceId] = j;
	}
	return j->m_index;
}

// ================================================================= //


#ifdef LINUX
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <cstring>

#define test_bit(nr, addr) ((1 << (nr&0x1f)) & addr[nr>>5])

// File must be a valid open input
inline bool isValidJoystick(int file, int bufSize, uint* evBit, uint* keyBit, uint* absBit) {
	if( ioctl(file, EVIOCGBIT(0,      bufSize), evBit) < 0 ) return false;
	if( ioctl(file, EVIOCGBIT(EV_KEY, bufSize), keyBit) < 0 ) return false;
	if( ioctl(file, EVIOCGBIT(EV_ABS, bufSize), absBit) < 0 ) return false;
	
	if( !test_bit(EV_KEY, evBit) || !test_bit(EV_ABS, evBit)) return false;
	if( !test_bit(ABS_X, absBit) || !test_bit(ABS_Y, absBit)) return false;
	if( !test_bit(BTN_TRIGGER, keyBit) && !test_bit(BTN_A, keyBit) && !test_bit(BTN_1, keyBit)) return false;
	return true;
}

int Input::initialiseJoysticks() {
	int js;
	unsigned int evBit[40];
	unsigned int keyBit[40];
	unsigned int absBit[40];
	unsigned int values[5];
	char filename[64];
	
	// Scan
	for(int i=0; i<32; ++i) {
		sprintf(filename, "/dev/input/event%d", i);
		js = open(filename, O_RDONLY);
		if(js < 0) continue;
		if(isValidJoystick(js, sizeof(evBit), evBit, keyBit, absBit)) {
			// valid - create joystick object
			int btn = 0, axes=0;
			// Create input maps
			uint8* btnMap = new uint8[KEY_MAX - BTN_MISC];
			uint8* absMap = new uint8[ABS_MAX];
			memset(btnMap, 0xff, KEY_MAX-BTN_MISC);
			memset(absMap, 0xff, ABS_MAX);
			for(int j=BTN_JOYSTICK; j<KEY_MAX; ++j) {
				if(test_bit(j, keyBit)) btnMap[j-BTN_MISC] = btn++;
			}
			for(int j=BTN_MISC; j<BTN_JOYSTICK; ++j) {
				if(test_bit(j, keyBit)) btnMap[j-BTN_MISC] = btn++;
			}
			for(int j=0; j<ABS_MAX; ++j) {
				if(test_bit(j, absBit)) {
					if(j==ABS_HAT0X) j = ABS_HAT3Y; // skip hat
					else if(ioctl(js, EVIOCGABS(j), values) >= 0) absMap[j] = axes++;
				}
			}
			// Create joystick object
			Joystick* joy = new Joystick(axes, btn);
			joy->m_file = js;
			joy->m_keyMap = btnMap;
			joy->m_absMap = absMap;
			ioctl(js, EVIOCGNAME(sizeof(joy->m_name)), joy->m_name);
			fcntl(js, F_SETFL, O_NONBLOCK);
			printf("Joystick %s: %d axes, %d buttons\n", joy->m_name, joy->m_numAxes, joy->m_numButtons);
			addJoystick(joy);
			continue;
		}
		close(js);
	}
	return m_joysticks.size();
}

bool Joystick::update() {
	int len;
	uint code;
	input_event events[32];
	int lastb = m_buttons;
	while((len = read(m_file, events, sizeof(events)))>0) {
		if(!m_created) continue;
		len /= sizeof(input_event);
		for(int i=0; i<len; ++i) {
			code = events[i].code;
			switch(events[i].type) {
			case EV_KEY:
				code = code < BTN_MISC? 0xff: m_keyMap[code-BTN_MISC];
				if(code < m_numButtons) {
					if(events[i].value) m_buttons |= 1<<code;
					else m_buttons &= ~(1<<code);
				}
				break;
			case EV_ABS:
				switch(code) {
				case ABS_HAT0X: m_hat.x = events[i].value; break;
				case ABS_HAT0Y: m_hat.y = events[i].value; break;
				default:
					code = m_absMap[code];
					if(code < m_numAxes) {
						m_axis[code] = events[i].value;
						// Auto calibration
						if(m_axis[code] > m_range[code].max) m_range[code].max = m_axis[code];
						if(m_axis[code] < m_range[code].min) m_range[code].min = m_axis[code];
					}
					break;
				}
			}
		}
	}
	m_changed = lastb ^ m_buttons;
	m_created = true;
	return true;
}

#endif

// ================================================================= //

#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#include <regstr.h>
int Input::initialiseJoysticks() {
	JOYINFOEX joyInfo;
	JOYCAPS   joyCaps;
	MMRESULT  result;
	int max = joyGetNumDevs();
	for(int i=JOYSTICKID1; i<max; ++i) {
		joyInfo.dwSize = sizeof(joyInfo);
		joyInfo.dwFlags = JOY_RETURNALL;
		result = joyGetPosEx(i, &joyInfo);
		if(result == JOYERR_NOERROR) {
			result = joyGetDevCaps(i, &joyCaps, sizeof(joyCaps));
			if(result == JOYERR_NOERROR) {
				// valid
				Joystick* joy = new Joystick(joyCaps.wNumAxes, joyCaps.wNumButtons);
				joy->m_file = i;
				strcpy(joy->m_name, joyCaps.szPname);
				int axes = joyCaps.wNumAxes;
				if(axes>0) joy->m_range[0] = { (int)joyCaps.wXmin, (int)joyCaps.wXmax };
				if(axes>1) joy->m_range[1] = { (int)joyCaps.wYmin, (int)joyCaps.wYmax };
				if(axes>2) joy->m_range[2] = { (int)joyCaps.wZmin, (int)joyCaps.wZmax };
				if(axes>4) joy->m_range[3] = { (int)joyCaps.wUmin, (int)joyCaps.wUmax };
				if(axes>3) joy->m_range[4] = { (int)joyCaps.wRmin, (int)joyCaps.wRmax };
				if(axes>5) joy->m_range[5] = { (int)joyCaps.wVmin, (int)joyCaps.wVmax };
				for(int i=0; i<axes; ++i) joy->m_axis[i] = (joy->m_range[i].min + joy->m_range[i].max) / 2;
				printf("Joystick %s: %d axes, %d buttons\n", joy->m_name, joy->m_numAxes, joy->m_numButtons);
				for(int i=0; i<axes; ++i) printf("%d: %d (%d - %d)\n", i, joy->m_axis[i], joy->m_range[i].min, joy->m_range[i].max);
				addJoystick(joy);
			}
		}
	}
	return m_joysticks.size();
}

bool Joystick::update() {
	JOYINFOEX info;
	info.dwSize = sizeof(info);
	info.dwFlags = JOY_RETURNALL | JOY_RETURNPOVCTS;
	joyGetPosEx(m_file, &info);
	const DWORD returnFlags[6] = { JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, JOY_RETURNU, JOY_RETURNR, JOY_RETURNV };
	for(size_t i=0; i<m_numAxes; ++i) {
		if(info.dwFlags & returnFlags[i]) {
			m_axis[i] = *((&info.dwXpos) + i);	// Assuming contiguous
		}
	}
	//printf("%lu %lu %lu %lu %lu %lu\n", info.dwXpos, info.dwYpos, info.dwZpos, info.dwUpos, info.dwRpos, info.dwVpos);
	if(info.dwFlags & JOY_RETURNBUTTONS) {
		m_changed = m_buttons ^ info.dwButtons;
		m_buttons = info.dwButtons;
	}
	if(info.dwPOV > 36000) m_hat.x = m_hat.y = 0;
	else {
		int p = info.dwPOV;
		m_hat.x = p == 0 || p == 18000? 0: p < 18000? 1: -1;
		m_hat.y = p == 9000 || p == 27000? 0: p > 9000 && p < 270? -1: 1;
	}
	return true;
}

#endif

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>

int Input::initialiseJoysticks() {
	printf("Initialising joysticks\n");
	if(emscripten_sample_gamepad_data() != EMSCRIPTEN_RESULT_SUCCESS) return 0;
	int num = emscripten_get_num_gamepads();
	printf("Detected %d joysticks\n", num);
	for(Joystick* joy: m_joysticks) delete joy;
	m_joysticks.clear();
	EmscriptenGamepadEvent state;
	for(int i=0; i<num; ++i) {
		if(emscripten_get_gamepad_status(i, &state) == EMSCRIPTEN_RESULT_SUCCESS) {
			printf("Detected controller: %s (%d axes, %d buttons)\n", state.id, state.numAxes, state.numButtons);
			Joystick* joy = new Joystick(state.numAxes, state.numButtons);
			joy->m_file = i;
			joy->setDeadzone(0.3);
			addJoystick(joy);
		}
	}
	return num;
}

bool Joystick::update() {
	EmscriptenGamepadEvent state;
	if(emscripten_sample_gamepad_data() != EMSCRIPTEN_RESULT_SUCCESS) return false;
	if(emscripten_get_gamepad_status(m_file, &state) == EMSCRIPTEN_RESULT_SUCCESS) {
		assert(m_numAxes == state.numAxes && m_numButtons==state.numButtons);
		uint buttons = 0;
		for(int i=0; i<m_numAxes; ++i) m_axis[i] = state.axis[i] * 100;
		for(int i=0; i<m_numButtons; ++i) buttons |= state.digitalButton[i] << i;
		m_changed = m_buttons ^ buttons;
		m_buttons = buttons;
		return true;
	}
	return false;
}
#endif


Joystick::Joystick(int a, int b) : m_index(-1u), m_numAxes(a), m_numButtons(b), m_dead(0.1), m_buttons(0), m_changed(0), m_file(-1), m_keyMap(0), m_absMap(0), m_created(false) {
	if(a>0) {
		m_axis = new int[a];
		m_range = new Range[a];
		for(int i=0; i<a; ++i) {
			m_range[i].min = -100;
			m_range[i].max =  100;
			m_axis[i] = 0;
		}
	}
	else m_axis=0, m_range=0;
}

Joystick::~Joystick() {
	#ifdef LINUX
	if(m_file>=0) close(m_file);
	#endif
	delete [] m_keyMap;
	delete [] m_absMap;
	delete [] m_axis;
	delete [] m_range;
}





