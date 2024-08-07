#pragma once

#include <base/gui/widgets.h>

namespace gui {
	class MenuBuilder {
		public:
		MenuBuilder(Root* root, const char* menuSkin, const char* itemSkin, const char* menuItemSkin=0, const char* iconSet=0) : m_root(root), m_button(itemSkin), m_menu(menuItemSkin) {
			if(!menuItemSkin) m_menu = itemSkin;
			m_popup = new Popup();
			if(menuSkin) m_popup->setSkin(root->getSkin(menuSkin));
			m_iconList = iconSet? root->getIconList(iconSet): nullptr;
		}
		~MenuBuilder() { for(MenuBuilder* m: m_subMenus) delete m; }
		template<class F>
		Button* addAction(const char* text, const char* icon, const F& lambda) {
			Widget* w = m_popup->addItem(m_root, m_button, text, text, [lambda](Button* b){
				Popup* menu = cast<Popup>(b->getParent());
				while(menu->getOwner()) menu = cast<Popup>(menu->getOwner());
				menu->hide();
				lambda();
			});
			Button* b = cast<Button>(w);
			if(icon && b) b->setIcon(m_iconList, icon);
			b->setAutosize(true);
			Point size = b->getPreferredSize();
			if(size.x > m_popup->getSize().x) m_popup->setSize(size.x, m_popup->getSize().y);
			return b;
		}
		template<class F>
		Button* addAction(const char* text, const F& lambda) {
			return addAction(text, nullptr, lambda);
		}
		MenuBuilder& addMenu(const char* text, const char* iconSet=nullptr) {
			MenuBuilder* b = new MenuBuilder(m_root, nullptr, m_button, m_menu);
			b->m_popup->setSkin(m_popup->getSkin());
			b->m_iconList = iconSet? m_root->getIconList(iconSet): m_iconList;
			Widget* btn = m_popup->addItem(m_root, m_menu, text, text);
			btn->eventMouseEnter.bind([menu=b->m_popup](Widget* b) {
				cast<Popup>(b->getParent())->hideOwnedPopups();
				if(b->isEnabled()) menu->popup(b, Popup::RIGHT);
			});
			btn->eventMouseExit.bind([menu=b->m_popup](Widget* w) {
				if(!menu->getAbsoluteRect().contains(w->getRoot()->getMousePos())) menu->hide();
			});
			m_subMenus.push_back(b);
			m_popup->addOwnedPopup(b->menu());
			return *b;
		}
		Popup* menu() { return m_popup; };
		operator Popup*() { return m_popup; }

		private:
		Root* m_root;
		const char* m_button;
		const char* m_menu;
		IconList* m_iconList;
		Popup* m_popup;
		std::vector<MenuBuilder*> m_subMenus;
	};
}

