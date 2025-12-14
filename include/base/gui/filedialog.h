#pragma once
// File dialogue widget functionality. Needs a valid template.

#include <base/gui/widgets.h>

namespace gui {
	class ListItem;
	class Listbox;
	class Textbox;

	// File picker dialog
	class FileDialog : public gui::Window {
		WIDGET_TYPE(FileDialog);
		FileDialog();
		~FileDialog();
		static FileDialog* create(gui::Root*);

		void setDirectory(const char* dir);
		void setFileName(const char* name);
		void setFilter(const char* filter);
		void setDefaultExtension(const char* ext);

		const char* getFile() const { return m_filename; } 
		const char* getDirectory() const { return m_history[m_historyIndex]; }

		/** Option panel */
		void         setOptionsWidth(int w);
		gui::Widget* getOptionsWidget() const;

		/** Show the dialog */
		void showOpen();
		void showSave();

		public:
		Delegate<void(const char*)> eventConfirm;

		protected:
		void  makeFileName();									// Create filename string
		void  refreshFileList();								// Update file list
		bool  match(const char* string, const char* pattern);	// String pattern matching
		String m_filename;				// Full file path
		String m_filter;				// File filter
		String m_extension;				// Default file extension
		std::vector<String> m_history;	// Dialog history
		uint  m_historyIndex;			// Position in history list of current directory (for forward)
		float m_lastClick; 				// Time of last click to detect double clicking - shoule be in base or gui

		protected:
		void initialise(const gui::Root*, const gui::PropertyMap&);
		gui::Listbox* m_list = nullptr;
		gui::Textbox* m_dir = nullptr;
		gui::Textbox* m_file = nullptr;
		gui::Button*  m_confirm = nullptr;
		gui::Widget*  m_options = nullptr;
		int m_folderIcon = 0;
		int m_fileIcon = 0;
		bool m_saveMode = false;

		void selectFile(gui::Listbox*, ListItem&);
		void clickFile(gui::Widget*, const Point&, int);
		void submitFileName(gui::Textbox*);
		void changedFileName(gui::Textbox*, const char*);
		void changedDirectory(gui::Textbox*);
		void pressUp(gui::Button*);
		void pressBack(gui::Button*);
		void pressForward(gui::Button*);
		void pressConfirm(gui::Button*);
	};

}


