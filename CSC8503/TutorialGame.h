#pragma once
#include "GameTechRenderer.h"
#ifdef USEVULKAN
#include "GameTechVulkanRenderer.h"
#endif
#include "PhysicsSystem.h"

#include "StateGameObject.h"
#include "BehaviourTreeObject.h"

namespace NCL {
	namespace CSC8503 {
		class Goat;
		class TutorialGame		{
		public:
			TutorialGame(bool local);
			~TutorialGame();

			virtual void UpdateGame(float dt);

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld(bool local);

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/
			void InitGameExamples();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, int numIsles, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void AddMazeToWorld(string fileName, Vector3 zeroPos = Vector3(0, 0, 0));
			void AddBridgeToWorld(Vector3 from, Vector3 to);

			void AddRandomSpheres();

			void InitWalls();
			void InitDefaultFloor();
			void AddGrassFloor();

			void GoatControl(float dt, Goat* goat, int cameraYaw, bool up, bool down, bool left, bool right);

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();

			GameObject* AddWallToWorld(const Vector3& position, float height, float width, bool alongX);
			GameObject* AddFloorToWorld(const Vector3& position, TextureBase* texture, Vector3 floorSize = Vector3(200, 2, 200));
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f, string name = "");
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f, string name = "");
			GameObject* AddHedgeToWorld(const Vector3& position, Vector3 dimensions, string name = "");

			Goat* AddPlayerToWorld();
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);

			StateGameObject* AddCivilianToWorld(const Vector3& position);
			void AddCiviliansToWorld(int amount);
			StateGameObject* AddMagicBallToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			void AddMagicBallsToWorld(int amount);

			BehaviourTreeObject* AddGooseToWorld(const Vector3& position);

			std::vector<StateGameObject*> stateGameObjects;
			std::vector<BehaviourTreeObject*> behaviourTreeObjects;

#ifdef USEVULKAN
			GameTechVulkanRenderer*	renderer;
#else
			GameTechRenderer* renderer;
#endif
			PhysicsSystem*		physics;
			GameWorld*			world;

			bool local;
			int finalScore;

			bool useGravity;
			bool inSelectionMode;
			bool goatControlMode;

			int highScore;

			float		forceMagnitude;

			float		gameDurationRemaining;

			GameObject* selectionObject = nullptr;

			Goat* goat;

			MeshGeometry*	capsuleMesh = nullptr;
			MeshGeometry*	cubeMesh	= nullptr;
			MeshGeometry*	sphereMesh	= nullptr;

			TextureBase*	basicTex	= nullptr;
			ShaderBase*		basicShader = nullptr;

			TextureBase* dogeTex = nullptr;

			TextureBase* grassTex = nullptr;
			TextureBase* hedgeTex = nullptr;

			//Coursework Meshes
			MeshGeometry*	charMesh	= nullptr;
			MeshGeometry*	enemyMesh	= nullptr;
			MeshGeometry*	gooseMesh	= nullptr;
			MeshGeometry*	bonusMesh	= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			GameObject* objClosest = nullptr;
		};
	}
}

