#pragma once
#include "Vector2.h"
#include "CollisionDetection.h"
#include "Debug.h"
#include "SimpleObserver.h"
#include "OctTree.h"

namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {
		template<typename T>
		class OctTree;

		template<typename T>
		struct OctTreeEntry {
			Vector3 pos;
			Vector3 size;
			T* object;
			bool slept;

			OctTreeEntry(T* obj, Vector3 pos, Vector3 size, bool slept) : object(obj), pos(pos), size(size), slept(slept) {}
		};

		template<typename T>
		class OctTreeNode {
		public:
			friend class OctTree<T>; // Needed for tree to call functions on root
			typedef std::function<void(std::list<OctTreeEntry<T>>&)> OctTreeFunc;

			void Delete(T* object) {
				for (auto it = contents.begin(); it != contents.end(); it++) {
					OctTreeEntry<T> entry = (*it);
					if (entry.object == object) {
						//std::cout << " object successfully removed\n";
						contents.erase(it);
						break;
					}
				}
			}

			void Update(T* object) {
				Vector3 objectPos = object->GetTransform().GetPosition();
				Vector3 objectSize;
				object->GetBroadphaseAABB(objectSize);
				if (CollisionDetection::AABBFullyWithin(objectPos, position, objectSize, size)) { // If it's fully in this node there's no need to update...
					object->AddOwningNode(this);
					return;
				}
				for (auto it = contents.begin(); it != contents.end(); it++) {
					OctTreeEntry<T> entry = (*it);
					if (entry.object == object) {
						contents.erase(it);
						//std::cout << " node " << this << " attempting to update object " << entry.object->GetName() << "...";
						OctTreeNode<T>* nodeToInsertInto = this;
						//std::cout << "Checking against node " << entry.object->GetName() << "...";
						while (!CollisionDetection::AABBFullyWithin(objectPos, nodeToInsertInto->position, objectSize, nodeToInsertInto->size)) {
							//std::cout << " object not fully in this node...";
							nodeToInsertInto = nodeToInsertInto->parent;
							//std::cout << " now will check against node " << nodeToInsertInto << "...";
							if (nodeToInsertInto == nullptr) // Edge case checking - has object left the whole tree?
								break;
						}
						if (nodeToInsertInto != nullptr) {
							//std::cout << " will insert at node " << nodeToInsertInto << "\n";
							nodeToInsertInto->Insert(object, objectPos, objectSize, false);
						}
						else {
							std::cout << "Object outside of OctTree, likely fell through map?\n";
						}
						return;
					}
				}
			}

		protected:
			OctTreeNode() {
				//std::cout << "Default OctTreeNode made, this was not passed a baseTree pointer, but baseTree is at memory location " << this->baseTree << std::endl;
			};
			OctTreeNode(OctTree<T>* baseTree, Vector3 position, Vector3 size, OctTreeNode<T>* parent, int depth) : baseTree(baseTree), position(position), size(size), parent(parent), depth(depth) {
				//std::cout << "OctTreeNode made, baseTree is at memory location " << this->baseTree << std::endl;
				children = nullptr;
				sleep = true;
			}
			~OctTreeNode() {
				//std::cout << "OctTreeNode deleted, baseTree was at memory location " << baseTree << std::endl;
				delete[] children;
			}

			void Insert(T* object, const Vector3& objectPos, const Vector3& objectSize, bool slept) {
				if (!CollisionDetection::AABBTest(objectPos, position, objectSize, size))
					return; // Object not in this node

				if (children)
					for (int i = 0; i < 8; i++) // This isn't a leaf node, descend the tree
						children[i].Insert(object, objectPos, objectSize, slept);
				else {
					contents.push_back(OctTreeEntry<T>(object, objectPos, objectSize, slept));
					//std::cout << "Object " << object->GetName() << " inserted into node " << this << "...";
					object->AddOwningNode(this);
					sleep = sleep && slept; // One awake object makes the node awake
					if ((int)contents.size() > baseTree->maxSize && baseTree->maxDepth > depth) {
						if (!children) {
							Split();
							// Re-insert existing contents
							for (const auto& i : contents) {
								auto entry = i;
								//std::cout << "Object " << entry.object->GetName() << " removed from node " << this << " (node split)...";
								entry.object->RemoveOwningNode(this);
								for (int j = 0; j < 8; j++) {
									children[j].Insert(entry.object, entry.pos, entry.size, entry.slept);
								}
							}
							contents.clear();
							sleep = true;
						}
					}
				}
			}

			void Split() {
				Vector3 halfSize = size / 2.0f;
				children = new OctTreeNode<T>[8];
				children[0] = OctTreeNode<T>(baseTree, position + Vector3(-halfSize.x, halfSize.y, halfSize.z), halfSize, this, depth + 1);
				children[1] = OctTreeNode<T>(baseTree, position + Vector3(halfSize.x, halfSize.y, halfSize.z), halfSize, this, depth + 1);
				children[2] = OctTreeNode<T>(baseTree, position + Vector3(-halfSize.x, halfSize.y, -halfSize.z), halfSize, this, depth + 1);
				children[3] = OctTreeNode<T>(baseTree, position + Vector3(halfSize.x, halfSize.y, -halfSize.z), halfSize, this, depth + 1);
				children[4] = OctTreeNode<T>(baseTree, position + Vector3(-halfSize.x, -halfSize.y, halfSize.z), halfSize, this, depth + 1);
				children[5] = OctTreeNode<T>(baseTree, position + Vector3(halfSize.x, -halfSize.y, halfSize.z), halfSize, this, depth + 1);
				children[6] = OctTreeNode<T>(baseTree, position + Vector3(-halfSize.x, -halfSize.y, -halfSize.z), halfSize, this, depth + 1);
				children[7] = OctTreeNode<T>(baseTree, position + Vector3(halfSize.x, -halfSize.y, -halfSize.z), halfSize, this, depth + 1);
				/*std::cout << "Node " << this << " has split into nodes";
				for (int i = 0; i < 8; i++)
					std::cout << " Node " << &children[i];
				std::cout << "\n";*/
			}

			void DebugDraw() {
				if (children)
					for (int i = 0; i < 8; i++)
						children[i].DebugDraw();
				else {

					Vector3 pos(position.x, position.y, position.z);
					Vector3 vertexes[8] = {
						pos + Vector3(size.x, size.y, size.z),
						pos + Vector3(size.x, size.y, -size.z),
						pos + Vector3(-size.x, size.y, size.z),
						pos + Vector3(-size.x, size.y, -size.z),
						pos + Vector3(size.x, -size.y, size.z),
						pos + Vector3(size.x, -size.y, -size.z),
						pos + Vector3(-size.x, -size.y, size.z),
						pos + Vector3(-size.x, -size.y, -size.z)
					};

					Debug::DrawLine(vertexes[0], vertexes[1]);
					Debug::DrawLine(vertexes[0], vertexes[2]);
					Debug::DrawLine(vertexes[0], vertexes[4]);

					Debug::DrawLine(vertexes[1], vertexes[3]);
					Debug::DrawLine(vertexes[1], vertexes[5]);

					Debug::DrawLine(vertexes[2], vertexes[3]);
					Debug::DrawLine(vertexes[2], vertexes[6]);

					Debug::DrawLine(vertexes[3], vertexes[7]);

					Debug::DrawLine(vertexes[4], vertexes[5]);
					Debug::DrawLine(vertexes[4], vertexes[6]);

					Debug::DrawLine(vertexes[5], vertexes[7]);

					Debug::DrawLine(vertexes[6], vertexes[7]);
				}
			}

			void OperateOnContents(OctTreeFunc& func, bool ignoreSleep = false) {
				if (children)
					for (int i = 0; i < 8; i++)
						children[i].OperateOnContents(func);
				else if (!contents.empty())
					if (!ignoreSleep || !sleep)
						func(contents);
			}

			void Clear() {
				if (children)
					for (int i = 0; i < 8; i++)
						children[i].Clear();
				else if (!contents.empty()) {
					contents.clear();
					sleep = true;
				}
			}

		protected:
			int depth;

			Vector3 position;
			Vector3 size;
			bool sleep;

			OctTree<T>* baseTree;
			std::list<OctTreeEntry<T>> contents;
			OctTreeNode<T>* parent;
			OctTreeNode<T>* children;
		};
	}
}