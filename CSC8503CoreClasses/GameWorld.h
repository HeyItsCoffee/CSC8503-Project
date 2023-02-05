#pragma once
#include <random>

#include "Ray.h"
#include "CollisionDetection.h"
#include "QuadTree.h"
#include "OctTree.h"
#include "NavigationGrid.h"
namespace NCL {
		class Camera;
		using Maths::Ray;
	namespace CSC8503 {
		class GameObject;
		class Constraint;
		class PhysicsSystem;


		typedef std::function<void(GameObject*)> GameObjectFunc;
		typedef std::vector<GameObject*>::const_iterator GameObjectIterator;

		class GameWorld	{
		public:
			friend class PhysicsSystem;
			GameWorld();
			~GameWorld();

			void Clear();
			void ClearAndErase();

			void AddGameObject(GameObject* o);
			void RemoveGameObject(GameObject* o, bool andDelete = false);

			void AddGoat(GameObject* o);
			void RemoveGoat(GameObject* o, bool andDelete = false);

			void AddConstraint(Constraint* c);
			void RemoveConstraint(Constraint* c, bool andDelete = false);

			Camera* GetMainCamera() const {
				return mainCamera;
			}

			void ShuffleConstraints(bool state) {
				shuffleConstraints = state;
			}

			void ShuffleObjects(bool state) {
				shuffleObjects = state;
			}

			bool Raycast(Ray& r, RayCollision& closestCollision, bool closestObject = false, GameObject* ignore = nullptr) const;

			virtual void UpdateWorld(float dt);

			void OperateOnContents(GameObjectFunc f);

			void GetObjectIterators(
				GameObjectIterator& first,
				GameObjectIterator& last) const;

			void GetConstraintIterators(
				std::vector<Constraint*>::const_iterator& first,
				std::vector<Constraint*>::const_iterator& last) const;

			int GetWorldStateID() const {
				return worldStateCounter;
			}

			void SetPhysicsSystem(PhysicsSystem* physics) {
				physicsSystem = physics;
			}

			void AddMaze(NavigationGrid* maze);
			NavigationGrid* GetMaze() const { return maze; }
			void RemoveMazeNode(GameObject* node);
			vector<Vector3> CalculatePathInMaze(Vector3 startPos, Vector3 endPos);

			vector<GameObject*> GetGoats() const { return playerGoats; }
			vector<GameObject*> GetGoatsInMaze();

		protected:
			std::vector<GameObject*> gameObjects;
			std::vector<GameObject*> playerGoats;
			std::vector<Constraint*> constraints;

			PhysicsSystem* physicsSystem;

			NavigationGrid* maze;

			Camera* mainCamera;

			bool shuffleConstraints;
			bool shuffleObjects;
			int		worldIDCounter;
			int		worldStateCounter;
		};
	}
}

