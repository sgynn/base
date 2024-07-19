#include <base/gui/skin.h>
#include <base/gui/widgets.h>
#include <base/gui/lists.h>
#include <algorithm>

using namespace gui;

Table::Table()
	: m_headerTemplate(0), m_header(0), m_dataPanel(0)
	, m_sortable(false), m_sorted(0), m_rowHeight(20), m_cacheOffset(0)
{}

Table::~Table() {
}

void Table::initialise(const Root* r, const PropertyMap& p) {
	m_header = getTemplateWidget("_header");
	m_headerTemplate = getTemplateWidget("_headeritem");
	m_dataPanel = getTemplateWidget<Scrollpane>("_data");
	m_dataPanel->getTemplateWidget("_client")->setSkin(0);	// Need a better way of overriding skins
	m_dataPanel->setSkin(0);
	m_dataPanel->useFullSize(true);
	m_dataPanel->setAutosize(false);
	bool hasHeader = !p.contains("header") || atoi(p["header"]);
	if(p.contains("rowheight")) m_rowHeight = atoi(p["rowheight"]);

	if(m_headerTemplate) m_headerTemplate->setVisible(false);
	if(m_header) m_header->setVisible(hasHeader);

	// No idea how to get column header data here. perhaps items ? or add another tag to copy in
	
}

void Table::draw() const {
	Widget::draw();
	// Possibly selected box, row striping, lines ?
}

void Table::copyData(const Widget* from) {
	if(const Table* table = cast<Table>(from)) {
		// Copy header data
		m_columns = table->m_columns;
		for(uint i=0; i<m_columns.size(); ++i) {
			m_columns[i].header = table->m_header->getWidget(i);
		}
	}
}

void Table::setSize(int w, int h) {
	Widget::setSize(w, h);
	// Resize row cache
	updateScrollSize();
}

void Table::showHeader(bool v) {
	if(m_header) m_header->setVisible(v);
}

void Table::setRowHeight(int h) {
	if(h!=m_rowHeight) {
		m_rowHeight = h;
		cacheAll();
	}
}

// ===================================================================== //

void Table::onMouseButton(const Point&, int, int) {
	// Select row
}

// ===================================================================== //

uint Table::getColumnCount() const {
	return m_columns.size();
}
const char* Table::getColumnName(uint i) const {
	return i<m_columns.size()? m_columns[i].name.str(): 0;
}
uint Table::getColumnIndex(const char* name) const {
	for(uint i=0; i<m_columns.size(); ++i) {
		if(m_columns[i].name == name) return i;
	}
	return ~0u;
}

uint Table::addColumn(const char* name, const char* title, int width, const char* itemWidget) {
	return insertColumn(m_columns.size(), name, title, width, itemWidget);
}

uint Table::insertColumn(uint index, const char* name, const char* title, int width, const char* itemWidget) {
	removeColumn(getColumnIndex(name)); // No duplicates
	if(index>m_columns.size()) index = m_columns.size();

	Column c;
	c.header = m_header && m_headerTemplate? m_headerTemplate->clone(): 0;
	c.name = name;
	c.width = width;
	c.widgets = itemWidget;
	c.pos = 0;
	for(uint i=0; i<index; ++i) c.pos += m_columns[i].width;
	m_columns.insert(m_columns.begin() + index, c);
	if(c.header) {
		m_header->add(c.header, c.pos, 0);
		c.header->setVisible(true);
		c.header->setSize(width, c.header->getSize().y);
		Label* lbl = cast<Label>(c.header);
		if(lbl) lbl->setCaption(title);
		Button* btn = cast<Button>(c.header);
		if(btn) btn->eventPressed.bind(this, &Table::columnPressed);
	}

	// TODO: Cache data in new column
	
	updateScrollSize();
	return index;
}

void Table::removeColumn(uint index) {
	if(index>=m_columns.size()) return;
	m_columns[index].header->removeFromParent();
	for(uint i=index+1; i<m_columns.size(); ++i) m_columns[i].pos -= m_columns[index].width;
	m_columns.erase(m_columns.begin() + index);
	// TODO: remove cached widgets
	updateScrollSize();
}

void Table::setColumnWidth(uint index, int width) {
	if(index>=m_columns.size()) return;
	if(m_columns[index].width == width) return;

	int delta = width - m_columns[index].width;
	m_columns[index].width = width;
	for(uint i=index+1; i<m_columns.size(); ++i) m_columns[i].pos += delta;

	if(m_columns[index].header) {
		m_columns[index].header->setSize(width, m_columns[index].header->getSize().y);
	}
	// resize and move all item widgets
	for(RowCache& row : m_cellCache) {
		row[index]->setSize( width, m_rowHeight );
		for(uint i=index+1; i<row.size(); ++i) {
			Point pos = row[i]->getPosition();
			row[i]->setPosition(pos.x + delta, pos.y);
		}
	}
	updateScrollSize();
}

void Table::setColumnTitle(uint index, const char* title) {
	if(index>=m_columns.size() || !m_columns[index].header) return;
	Label* lbl = cast<Label>(m_columns[index].header);
	if(lbl) lbl->setCaption(title);
}

void Table::setColumnIndex(uint index, uint newIndex) {
	if(newIndex >= m_columns.size()) newIndex = m_columns.size() - 1;
	if(index>=m_columns.size() || newIndex==index) return;
	Column c = m_columns[index];
	m_columns.erase(m_columns.begin()+index);
	m_columns.insert(m_columns.begin()+newIndex, c);
	// TODO: update item cache
}

int Table::getColumnWidth(uint index) const {
	return index<m_columns.size()? m_columns[index].width: 0;
}

// --------------------------------------------------------------------------------------- //

uint Table::addRow() {
	return addCustomRow(DefaultRowType());
}

uint Table::insertRow(uint index) {
	return insertCustomRow(index, DefaultRowType());
}
uint Table::addCustomRow(const Any& row) {
	m_data.push_back(row);
	cacheRow(m_data.size()-1);
	updateScrollSize();
	return m_data.size() - 1;
}

uint Table::insertCustomRow(uint index, const Any& data) {
	if(index>=m_data.size()) return addCustomRow(data);
	else m_data.insert(m_data.begin()+index, data);
	cacheRow(index);
	updateScrollSize();
	return index;
}

void Table::setCustomRow(uint row, const Any& data) {
	if(row>=m_data.size()) addCustomRow(data);
	else {
		m_data[row] = data;
		cacheRow(row);
	}
}

const Any& Table::getRowData(uint row) const {
	static const Any nope;
	return row<m_data.size()? m_data[row]: nope;
}

void Table::removeRow(uint index) {
	if(index<m_data.size()) {
		m_data.erase(m_data.begin()+index);
		// Update cached widgets
		if(index>=m_cacheOffset && index-m_cacheOffset<m_cellCache.size()) {
			int from = index - m_cacheOffset;
			for(uint i=from+1; i<m_cellCache.size(); ++i) std::swap(m_cellCache[i-1], m_cellCache[i]);
			cachePositions(index);
		}
		updateScrollSize();
	}
}

void Table::clearRows() {
	m_data.clear();
	clearCache();
	updateScrollSize();
}

uint Table::getRowCount() const { 
	return m_data.size();
}

Point Table::getCellAt(const Point& pos, bool global) const {
	Point p = global? pos - m_client->getAbsolutePosition(): pos;
	Point result(0, pos.y / m_rowHeight);
	while(result.x < (int)getColumnCount() && p.x >= getColumnWidth(result.x)) {
		p.x -= getColumnWidth(result.x);
		++result.x; 
	}
	return result;
}

bool Table::setValue(uint row, uint column, const Any& value) {
	if(row<getRowCount() && column<getColumnCount()) {
		DefaultRowType* rowData = m_data[row].cast<DefaultRowType>();
		if(rowData) {
			if(column >= rowData->size()) rowData->resize(getColumnCount(), Any());
			rowData->at(column) = value;
			cacheItem(row, column);
			return true;
		}
		else printf("Error: Can't use Table::setValue when using custom row types\n");
	}
	return false;
}
const Any& Table::getValue(uint row, uint column) const {
	static const Any nope;
	if(row<getRowCount() && column<getColumnCount()) {
		DefaultRowType* rowData = m_data[row].cast<DefaultRowType>();
		if(rowData) {
			if(column >= rowData->size()) return nope;
			return rowData->at(column);
		}
		else printf("Error: Can't use Table::getValue when using custom row types\n");
	}
	return nope;
}

// --------------------------------------------------------------------------------------- //

void Table::columnPressed(Button* b) {
	if(eventColummPressed) eventColummPressed(this, b->getIndex());
}

void Table::fireCustomEvent(Widget* w) {
	if(!eventCustom) return;
	Point p = w->getPosition();
	int row = p.y / m_rowHeight;
	int col = 0;
	while(p.x>m_columns[col].pos) ++col;
	eventCustom(this, row, col, w);
}

// --------------------------------------------------------------------------------------- //

void Table::updateScrollSize() {
	if(!m_dataPanel) return;
	if(m_data.empty()) m_dataPanel->setPaneSize(0,0);
	else m_dataPanel->setPaneSize( m_columns.back().pos + m_columns.back().width, m_data.size() * m_rowHeight);
}

inline const char* anyToString(const Any& v) {
	if(v.isType<String>()) return *v.cast<String>();
	else if(v.isType<const char*>()) return *v.cast<const char*>();
	else if(v.isType<char*>()) return *v.cast<const char*>();
	else if(v.isType<bool>()) return *v.cast<bool>()? "True": "False";
	else {
		static char buffer[64];
		buffer[0] = 0;
		if(v.isType<int>()) sprintf(buffer, "%d", *v.cast<int>());
		else if(v.isType<float>()) sprintf(buffer, "%g", *v.cast<float>());
		else if(v.isType<double>()) sprintf(buffer, "%g", *v.cast<double>());
		else return "??";
		return buffer;
	}
}

void Table::onAdded() {
	cacheAll();
}

void Table::clearCache() {
	m_dataPanel->deleteChildWidgets();
	m_cellCache.clear();
}

void Table::cacheAll() {
	if(!getRoot()) return;
	for(uint i=0; i<m_data.size(); ++i) {
		for(uint j=0; j<m_columns.size(); ++j) {
			cacheItem(i, j);
		}
	}
}

void Table::cacheRow(uint index) {
	if(!getRoot()) return;
	for(uint i=0; i<m_columns.size(); ++i) {
		cacheItem(index, i);
	}
}

void Table::cacheItem(uint row, uint column) {
	if(!getRoot()) return;
	if(row>=m_cellCache.size()) m_cellCache.resize(row+1);
	RowCache& rowWidgets = m_cellCache[row];
	if(rowWidgets.size() < m_columns.size()) rowWidgets.resize(m_columns.size(), 0);
	Widget*& w = rowWidgets[column];

	// Create widget
	if(!w) {
		int ypos = row * m_rowHeight;
		const Column& col = m_columns[column];
		const Widget* templateWidget = col.widgets? getRoot()->getTemplate(col.widgets): 0;
		if(templateWidget) {
			w = templateWidget->clone();
			w->setSize(col.width, m_rowHeight);
			w->setPosition(col.pos, ypos);

			if(Checkbox* c=cast<Checkbox>(w)) c->eventChanged.bind([this](Button* b){fireCustomEvent(b); });
			else if(Button* b=cast<Button>(w)) b->eventPressed.bind([this](Button* b){fireCustomEvent(b); });
			else if(Textbox* t=cast<Textbox>(w)) t->eventSubmit.bind([this](Textbox* t){fireCustomEvent(t); });
			else if(Combobox* l=cast<Combobox>(w)) l->eventSelected.bind([this](Combobox* c, ListItem&){fireCustomEvent(c); });
		}
		if(!w) {
			w = new Label();
			w->setSkin(m_skin);
			w->setRect(col.pos, ypos, col.width, m_rowHeight);
		}
		m_dataPanel->add(w);
	}

	// Cache value
	if(!eventCacheItem || !eventCacheItem(this, row, column, w)) {
		const Any& value = getValue(row, column);
		if(cast<Checkbox>(w) && value.isType<bool>()) cast<Checkbox>(w)->setChecked(*value.cast<bool>());
		else if(cast<Label>(w)) cast<Label>(w)->setCaption(anyToString(value));
		else if(cast<Textbox>(w)) cast<Textbox>(w)->setText(anyToString(value));
		else if(cast<Combobox>(w)) cast<Combobox>(w)->setText(anyToString(value));
		else if(cast<Image>(w)) {
			if(value.isType<int>()) cast<Image>(w)->setImage(*value.cast<int>());
			else cast<Image>(w)->setImage(anyToString(value));
		}
	}
}

void Table::cachePositions(uint fromRow, uint fromCol) {
	uint bottom = m_cacheOffset + m_cellCache.size();
	uint cols = m_columns.size();
	if(fromCol >= cols || fromRow >= bottom) return;
	if(fromRow < m_cacheOffset) fromRow=m_cacheOffset;
	for(uint r=fromRow; r<bottom; ++r) {
		RowCache& row = m_cellCache[r-m_cacheOffset];
		for(uint c=fromCol; c<cols; ++c) {
			if(row[c]) {
				row[c]->setPosition(m_columns[c].pos, r*m_rowHeight);
				if(r >= m_data.size()) {
					row[c]->removeFromParent();
					delete row[c];
					row[c] = 0;
				}
			}
		}
	}
}

