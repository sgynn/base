#pragma once

#include <vector>
#include <base/vec.h>
#include <base/quaternion.h>
#include <base/hashmap.h>

namespace base {
	class ModelLayout {
		public:
		enum NodeType { MARKER, MESH, INSTANCE, SHAPE, LIGHT };
		struct Node {
			NodeType type = MARKER;
			const char* name = nullptr;		// Object name
			const char* bone = nullptr;		// Bone node is attached to
			const char* object = nullptr;	// Mesh name, or object type
			base::HashMap<const char*> properties; // Custom properties
			Quaternion orientation;
			vec3 position;
			vec3 scale = vec3(1,1,1);
			Node* parent = nullptr;
			std::vector<Node*> children;
		};

		public:
		ModelLayout() { }
		~ModelLayout() {
			for(const Node& node: *this) {
				free((char*)node.name);
				free((char*)node.bone);
				free((char*)node.object);
				for(auto& p: node.properties) free((char*)p.value);
			}
		}
		const Node* findNode(const char* name) const {
			if(!name) return nullptr;
			for(const Node& node: *this) if(node.name && strcmp(name, node.name)==0) return &node;
			return nullptr;
		}
		static void getDerivedTransform(const Node* node, vec3& pos, Quaternion& rot) {
			pos = node->position;
			rot = node->orientation;
			for(Node* n = node->parent; n->parent; n=n->parent) {
				pos = n->position + n->orientation * pos;
				rot = n->orientation * rot;
			}
		}
		const Node& root() const { return m_root; }
		Node& root() { return m_root; }

		// Depth first tree iterator
		class Iterator {
			public:
			const Node* node;
			std::vector<size_t> stack;
			std::vector<Node*>::const_iterator current;
			Iterator(const Node& n) : node(&n), current(n.children.begin()) {}
			bool operator==(const Iterator& i) const { return current == i.current; }
			bool operator!=(const Iterator& i) const { return current != i.current; }
			Iterator& operator++() {
				if(!(*current)->children.empty()) {
					stack.push_back(*current - *node->children.begin());
					node = *current;
					current = node->children.begin();
				}
				else {
					++current;
					if(current == node->children.end() && !stack.empty()) {
						node = node->parent;
						current = node->children.begin() + stack.back() + 1;
						stack.pop_back();
					}
				}
				return *this;
			}
			const Node& operator*() const { return *(*current); }
			const Node* operator->() const { return *current; }
		};
		Iterator begin() const { return Iterator(m_root); }
		Iterator end() const { Iterator e(m_root); e.current=m_root.children.end(); return e; }

		private:
		Node m_root;
	};
}




