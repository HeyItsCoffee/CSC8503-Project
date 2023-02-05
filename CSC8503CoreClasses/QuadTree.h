#pragma once
#include "Vector2.h"
#include "CollisionDetection.h"
#include "Debug.h"

namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {
		template<class T>
		class QuadTree;

		template<class T>
		struct QuadTreeEntry {
			Vector3 pos;
			Vector3 size;
			T object;
			bool slept;

			QuadTreeEntry(T obj, Vector3 pos, Vector3 size, bool slept) {
				object		= obj;
				this->pos	= pos;
				this->size	= size;
				this->slept = slept;
			}
		};

		template<class T>
		class QuadTreeNode	{
		public:
			typedef std::function<void(std::list<QuadTreeEntry<T>>&)> QuadTreeFunc;
		protected:
			friend class QuadTree<T>;

			QuadTreeNode() {}

			QuadTreeNode(Vector2 pos, Vector2 size, QuadTreeNode<T>* parent) {
				children		= nullptr;
				this->position	= pos;
				this->size		= size;
				this->sleep		= true;
				this->parent	= parent;
			}

			~QuadTreeNode() {
				delete[] children;
			}

			void Insert(T& object, const Vector3& objectPos, const Vector3& objectSize, int depthLeft, int maxSize, bool slept) {
				if (!CollisionDetection::AABBTest(objectPos, Vector3(position.x, 0, position.y), objectSize, Vector3(size.x, 1000.0f, size.y)))
					return;

				if (children) // Not a leaf node, just descend the tree
					for (int i = 0; i < 4; i++)
						children[i].Insert(object, objectPos, objectSize, depthLeft - 1, maxSize, slept);
				else { // This is a leaf node, just expand
					contents.push_back(QuadTreeEntry<T>(object, objectPos, objectSize, slept));
					if (!slept)
						sleep = false;
					if((int)contents.size() > maxSize && depthLeft > 0)
						if (!children) {
							Split();
							// Need to re-insert contents so far
							for(const auto& i : contents)
								for (int j = 0; j < 4; j++) {
									auto entry = i;
									children[j].Insert(entry.object, entry.pos, entry.size, depthLeft - 1, maxSize, entry.slept);
								}
							contents.clear(); // Contents are now distributed between children, delete from here
							sleep = true;
						}
				}
			}

			void Split() {
				Vector2 halfSize = size / 2.0f;
				children = new QuadTreeNode<T>[4];
				children[0] = QuadTreeNode<T>(position + Vector2(-halfSize.x, halfSize.y), halfSize, this);
				children[1] = QuadTreeNode<T>(position + Vector2(halfSize.x, halfSize.y), halfSize, this);
				children[2] = QuadTreeNode<T>(position + Vector2(-halfSize.x, -halfSize.y), halfSize, this);
				children[3] = QuadTreeNode<T>(position + Vector2(halfSize.x, -halfSize.y), halfSize, this);
			}

			void DebugDraw() {
				if (children)
					for (int i = 0; i < 4; i++)
						children[i].DebugDraw();

				Vector3 pos(position.x, 0, position.y);
				Vector3 vertexes[8] = {
					pos + Vector3(size.x, 10, size.y),
					pos + Vector3(size.x, 10, -size.y),
					pos + Vector3(-size.x, 10, size.y),
					pos + Vector3(-size.x, 10, -size.y),
					pos + Vector3(size.x, -20, size.y),
					pos + Vector3(size.x, -20, -size.y),
					pos + Vector3(-size.x, -20, size.y),
					pos + Vector3(-size.x, -20, -size.y)
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

			void OperateOnContents(QuadTreeFunc& func) {
				if (children)
					for (int i = 0; i < 4; i++)
						children[i].OperateOnContents(func);
				else if (!contents.empty())
					if(!sleep)
						func(contents);
			}

			void Clear() {
				if (children)
					for (int i = 0; i < 4; i++)
						children[i].Clear();
				else if (!contents.empty()) {
					contents.clear();
					sleep = true;
				}
			}

		protected:
			std::list< QuadTreeEntry<T> >	contents;

			bool sleep;
			Vector2 position;
			Vector2 size;

			QuadTreeNode<T>* parent;
			QuadTreeNode<T>* children;
		};
	}
}


namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {
		template<class T>
		class QuadTree
		{
		public:
			QuadTree(Vector2 size = Vector2(1024, 1024), int maxDepth = 6, int maxSize = 5){
				root = QuadTreeNode<T>(Vector2(), size, nullptr);
				this->maxDepth	= maxDepth;
				this->maxSize	= maxSize;
				std::cout << "QuadTree made\n";
			}
			~QuadTree() {
				std::cout << "QuadTree deleted\n";
			}

			void Insert(T object, const Vector3& pos, const Vector3& size, bool slept = false) {
				root.Insert(object, pos, size, maxDepth, maxSize, slept);
			}

			void DebugDraw() {
				root.DebugDraw();
			}

			void OperateOnContents(typename QuadTreeNode<T>::QuadTreeFunc  func) {
				root.OperateOnContents(func);
			}

			// Remove all contents of the tree. Nodes that form the structure of the tree are preserved.
			void Clear() {
				root.Clear();
			}

		protected:
			QuadTreeNode<T> root;
			int maxDepth;
			int maxSize;
		};
	}
}