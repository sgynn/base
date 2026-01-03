#include <base/gui/filedialog.h>
#include <base/gui/lists.h>
#include <base/directory.h>
#include <cstring>
#include <cstdio>

using base::Directory;
using base::StringList;
using namespace gui;

FileDialog::FileDialog() {
}

FileDialog::~FileDialog() {
}

void FileDialog::initialise(const Root* r, const PropertyMap& p) {
	Window::initialise(r, p);
	if(m_client->getWidgetCount()==0) return; // Nothing

	// Cache sub widgets
	m_list = getWidget<Listbox>("filelist");
	m_file = getWidget<Textbox>("filename");
	m_dir  = getWidget<Textbox>("path");
	m_confirm = getWidget<Button>("confirm");
	m_options = getWidget<Widget>("options");

	if(!m_list || !m_file || !m_dir || !m_confirm) {
		printf("Error: File dialog template is missing components\n");
		return;
	}
	
	// Set up callbacks
	m_list->eventSelected.bind(this, &FileDialog::selectFile);
	m_list->eventMouseDown.bind(this, &FileDialog::clickFile);
	m_confirm->eventPressed.bind(this, &FileDialog::pressConfirm);
	m_file->eventChanged.bind(this, &FileDialog::changedFileName);
	m_file->eventSubmit.bind(this, &FileDialog::submitFileName);
	m_dir->eventSubmit.bind(this, &FileDialog::changedDirectory);
	
	
	if(Button* b = getWidget<Button>("up"))   b->eventPressed.bind(this, &FileDialog::pressUp);
	if(Button* b = getWidget<Button>("back")) b->eventPressed.bind(this, &FileDialog::pressBack);
	if(Button* b = getWidget<Button>("fwd"))  b->eventPressed.bind(this, &FileDialog::pressForward);

	// Load properties
	const char* initialPath = ".";
	if(p.contains("filter")) setFilter(p["filter"]);
	if(p.contains("dir")) initialPath = p["dir"];

	// Initial directory
	char buffer[FILENAME_MAX];
	Directory::getFullPath(initialPath, buffer);
	setDirectory(buffer);
}

void FileDialog::setOptionsWidth(int w) {
	if(!m_options) return;
	int full = m_list->getSize().x + m_options->getSize().x;
	int height = m_list->getSize().y;
	Point p = m_list->getPosition();
	m_list->setSize(full-w, height);
	m_options->setSize(w, height);
	m_options->setPosition(p.x + full - w, p.y);
	m_options->setVisible( w > 0 );
}
Widget* FileDialog::getOptionsWidget() const {
	return m_options;
}

void FileDialog::setFilter(const char* f) {
	if(f==0 || (f[0]=='*' && (f[1]==0 || f[1]==','))) m_filter.clear(); // No filter
	else m_filter = f;
	refreshFileList();
}

void FileDialog::setDirectory(const char* dir) {
	if(!m_history.empty() && strcmp(dir, getDirectory())==0) return; // no change
	printf("cd %s\n", dir);

	// Clear forward history
	while(m_history.size() > m_historyIndex + 1) {
		m_history.pop_back();
	}

	// Button states
	if(Widget* b = getWidget("back")) b->setEnabled(!m_history.empty());
	if(Widget* b = getWidget("fwd")) b->setEnabled(false);

	// Add directory to history
	char buffer[1024];
	Directory::clean(dir, buffer);
	m_history.push_back(buffer);
	m_historyIndex = m_history.size() - 1;
	
	// Update directory textbox
	if(m_dir) m_dir->setText(buffer);
	if(m_file) m_file->setText("");
	changedFileName(0,"");

	// Refresh
	refreshFileList();
}
void FileDialog::refreshFileList() {
	if(!isVisible()) return;
	m_list->clearItems();
	Directory d( getDirectory() );
	StringList filters = String::split(m_filter, ",");
	for(Directory::iterator i = d.begin(); i!=d.end(); ++i) {
		if(i->name[0]=='.') continue; // Ignore hidden files
		// Match filters
		if(i->type != Directory::DIRECTORY && !filters.empty()) {
			bool matches = false;
			for(const String& f: filters) {
				if(String::match(i->name, f)) {
					matches = true;
					break;
				}
			}
			if(!matches) continue;
		}
		// Add item to list
		if(i->type == Directory::DIRECTORY) m_list->addItem(i->name, "folder", true);
		else m_list->addItem(i->name, "file");
	}
}

void FileDialog::setFileName(const char* name) {
	m_file->setText(name);
}

void FileDialog::setDefaultExtension(const char* ext) {
	m_extension = ext;
	if(m_file) makeFileName();
}

void FileDialog::makeFileName() {
	const char* name = m_file->getText();
	const char* dir = m_dir->getText();
	// Check extension
	if(m_extension) {
		int sl = strlen(name);
		int el = m_extension.length();
		const char* e = name + sl - el - 1;
		// Add extension
		if(*e != '.' || strcmp(e+1, m_extension) != 0) {
			m_filename = String::format("%s/%s.%s", dir, name, m_extension);
			return;
		}
	}
	// Extension already there
	m_filename = String::format(m_filename, "%s/%s", dir, name);
}

void FileDialog::showOpen() {
	if(!m_list) return;
	setCaption("Open File");
	m_confirm->setCaption("Open");
	setVisible(true);
	refreshFileList();
	m_list->setFocus();
	m_confirm->setEnabled(m_file->getText()[0]);
	//m_file->select(0,999);
	m_saveMode = false;
	raise();
}
void FileDialog::showSave() {
	if(!m_list) return;
	setCaption("Save File");
	m_confirm->setCaption("Save");
	setVisible(true);
	refreshFileList();
	m_file->setFocus();
	m_confirm->setEnabled(m_file->getText()[0]);
	//m_file->select(0,999);
	m_saveMode = true;
	raise();
}

inline bool isDirectory(const ListItem& item) {
	return item.getData(2).getValue(false);
}

void FileDialog::selectFile(Listbox* list, ListItem& item) {
	m_file->setText(item);
	m_confirm->setEnabled( true );
	if(m_saveMode) {
		m_confirm->setCaption( isDirectory(item)? "Open": "Save" );
	}
}

void FileDialog::clickFile(Widget*, const Point& p, int b) {
	// Detect double click - root shoud have this
	static Point lp;
	if(lp==p && b==1) {
		pressConfirm(0);
		lp.x = -100;
	}
	else lp = p;
}

void FileDialog::changedDirectory(Textbox* t) {
	printf("change dir\n");
	setDirectory(t->getText());
}

void FileDialog::changedFileName(Textbox*, const char* t) {
	m_confirm->setEnabled(t[0]);
	if(m_saveMode) m_confirm->setCaption("Save");
}

void FileDialog::submitFileName(Textbox*) {
	pressConfirm(0);
}

void FileDialog::pressUp(Button*) {
	if(strlen(getDirectory()) < 3) return;
	char buffer[1024];
	sprintf(buffer, "%s/..", getDirectory());
	setDirectory(buffer);
}

void FileDialog::pressBack(Button* b) {
	if(m_historyIndex) {
		--m_historyIndex;
		refreshFileList();
		m_dir->setText( getDirectory() );
		b->setEnabled( m_historyIndex>0 );
		getWidget("fwd")->setEnabled(true);
	}
}

void FileDialog::pressForward(Button* b) {
	if(m_historyIndex < m_history.size()-1) {
		++m_historyIndex;
		refreshFileList();
		m_dir->setText( getDirectory() );
		b->setEnabled( m_historyIndex < m_history.size()-1 );
		getWidget("back")->setEnabled(true);
	}
}

void FileDialog::pressConfirm(Button*) {
	// Directory selected
	if(m_list->getSelectionSize() && isDirectory(*m_list->getSelectedItem())) {
		char buffer[1024];
		sprintf(buffer, "%s/%s", getDirectory(), m_list->getSelectedItem()->getText());
		setDirectory(buffer);
	}
	// Confirm file operation
	else {
		makeFileName();
		setVisible(false);
		if(eventConfirm) eventConfirm(m_filename);
	}
}

FileDialog* FileDialog::create(Root* gui) {
	// Create a default layout
	// Required templates: window, listbox, editbox, button, iconbutton
	Root::registerClass<FileDialog>();
	FileDialog* d = gui->createWidget<FileDialog>(Rect(100,100,500,228), "window");
	if(!d) {
		printf("Failed to create FileDialog: Missing template 'window'\n");
		return nullptr;
	}
	gui->getRootWidget()->add(d);
	d->createChild<Textbox>(Rect(0,0,428,20), "editbox", "path", "tlr");
	d->createChild<Button>(Rect(428,0,20,20), "iconbutton", "up", "tr");
	d->createChild<Button>(Rect(448,0,20,20), "iconbutton", "back", "tr");
	d->createChild<Button>(Rect(468,0,20,20), "iconbutton", "fwd", "tr");
	d->createChild<Listbox>(Rect(0,20,490,160), "listbox", "filelist", "tlrb");
	d->createChild<Textbox>(Rect(0,180,408,20), "editbox", "filename", "blr");
	d->createChild<Button>(Rect(0,180,80,20), "button", "confirm", "br");
	d->initialise(gui, {});
	d->setVisible(false);
	return d;
}



