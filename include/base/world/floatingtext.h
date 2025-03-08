#pragma once

#include <base/game.h>
#include <base/camera.h>
#include <base/gamestate.h>
#include <base/thread.h>
#include <list>

namespace gui { class Root; class Widget; }
namespace base {

class Camera;

/** Staticly acessible game component to easily add text floaters in world space. Thread safe.
 *	 - Add the game with addComponent(new FloatingText());
 *	 - Call FloatingText::add("Hello", 0xffffff, worldPosition);
 *	 - Optionally track a position (can be unsafe)
 *	
 *	Requires a GUIComponent with "label" widget template defined
 *	Requires a SceneComponent
 */
class FloatingText : public GameStateComponent {
	public:
	FloatingText(float duration=3, float speed=0.4);
	~FloatingText();
	void begin() override;
	void draw() override {}
	void update() override;
	static void add(const char* text, uint colour, const vec3& pos);
	static void add(const char* text, uint colour, const vec3& track, const vec3& offset);
	static void clear();

	private:
	bool setup();

	private:
	struct Item { gui::Widget* widget; vec3 pos; const vec3& track; float time; };
	std::list<Item> m_list;
	std::list<Item> m_new;
	gui::Root* m_gui = nullptr;
	gui::Widget* m_parent = nullptr;
	Camera* m_camera = nullptr;
	float m_duration = 3;
	float m_speed = 0.4;
	bool m_clearFlag = false;
	Mutex m_mutex;
	static FloatingText* s_instance;
};

}

