#pragma once

/** libbase audio engine interface */
namespace audio {
	typedef unsigned int soundID;
	typedef unsigned int objectID;

	int initialise(bool threaded=true);	// Initialise sound engine
	int shutdown();						// Shut down sound engine

	void load(const char* file);	// Load sound definition file and contained sounds
	void unload(const char* file);	// Unload sound bank

	soundID loadSound(const char* source, const char* bankName=0);	// Manually load a sound
	soundID getSoundID(const char* name);							// Get soundID from sound name

	void setValue(const char* variable, float value);				// Set global variable
	void setEnum (const char* variable, const char* value);			// Set global enum
	void setEnum (const char* variable, int value);					// set global enum

	void setValue(objectID object, const char* variable, float value);		// Set object variable
	void setEnum (objectID object, const char* variable, const char* value);// Set object enum
	void setEnum (objectID object, const char* variable, int value);		// Set object enum

	objectID createObject ();												// Create a new audio object
	void destroyObject(objectID object);									// Destroy audio object
	void setPosition  (objectID object, const float* position);				// Set object position
	void setPosition  (objectID object, const float* position, const float* direction);	// Set object position and direction
	void fireEvent    (objectID object, const char* event);						// Fire an event
	void playSound    (objectID object, const char* sound);						// Play a sound
	void playSound    (objectID object, soundID sound);							// Play a sound
	void stopSound    (objectID object, const char* sound);						// Stop a sound if playing
	void stopSound    (objectID object, soundID sound);							// Stop a sound if playing
	void stopAll      (objectID object);										// Stop all sounds on an object

	void setListener  (const float* position);								// Move listener


	void    createAttenuation(const char* bank, const char* name, float start, float end, int type);
	soundID createSound(const char* bank, const char* source, int loop=1, float volume=1, const char* mixer=0, const char* attenuation=0);
	soundID createGroup(const char* bank, const char* name, int type, const char* var=0);
	void    addToGroup(soundID group, soundID item, int key=0);


}

