#pragma once
#include "Vector2.h"
#include "CollisionDetection.h"
#include "Debug.h"
#include "SimpleObserver.h"
#include "OctTreeNode.h"

namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {
		template <typename T>
		class OctTree {
		public:
			friend class OctTreeNode<T>; // Needed for nodes to see the max depth and size
			OctTree() {
				//std::cout << "Default OctTree made: memory location is " << this << std::endl;
			}
			OctTree(Vector3 size, int maxDepth, int maxSize) : maxDepth(maxDepth), maxSize(maxSize){
				//std::cout << "OctTree made: memory location is " << this << std::endl;
				root = OctTreeNode<T>(this, Vector3(), size, nullptr, 1);
			}
			~OctTree() {
				//std::cout << "OctTree at memory location " << this << " deleted\n";
			}

			void Insert(T* object, Vector3& pos, Vector3& size, bool slept = false) {
				root.Insert(object, pos, size, slept);
			}

			void OperateOnContents(typename OctTreeNode<T>::OctTreeFunc func) {
				root.OperateOnContents(func);
			}

			void DebugDraw() {
				root.DebugDraw();
			}

			void Clear() {
				root.Clear();
			}

		protected:
			OctTreeNode<T> root;
			int maxDepth;
			int maxSize;
		};
	}
}