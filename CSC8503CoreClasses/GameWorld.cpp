#include "GameWorld.h"
#include "GameObject.h"
#include "RenderObject.h"
#include "Constraint.h"
#include "CollisionDetection.h"
#include "Camera.h"
#include "PhysicsSystem.h"


using namespace NCL;
using namespace NCL::CSC8503;

GameWorld::GameWorld()	{
	mainCamera = new Camera();

	shuffleConstraints	= false;
	shuffleObjects		= false;
	worldIDCounter		= 0;
	worldStateCounter	= 0;
}

GameWorld::~GameWorld()	{
}

void GameWorld::Clear() {
	gameObjects.clear();
	playerGoats.clear();
	constraints.clear();
	maze = nullptr;
	worldIDCounter		= 0;
	worldStateCounter	= 0;
}

void GameWorld::ClearAndErase() {
	for (auto& i : gameObjects) {
		delete i;
	}
	// playerGoats already cleared as they are in gameObjects too
	for (auto& i : constraints) {
		delete i;
	}
	delete maze;
	Clear();
}

void GameWorld::AddGameObject(GameObject* o) {
	gameObjects.emplace_back(o);
	o->SetWorldID(worldIDCounter++);
	worldStateCounter++;
	physicsSystem->AddObject(o);
}

void GameWorld::RemoveGameObject(GameObject* o, bool andDelete) {
	gameObjects.erase(std::remove(gameObjects.begin(), gameObjects.end(), o), gameObjects.end());
	physicsSystem->RemoveObject(o);
	if (andDelete) {
		delete o;
	}
	worldStateCounter++;
}

void GameWorld::AddGoat(GameObject* o) {
	playerGoats.emplace_back(o);
	AddGameObject(o);
}

void GameWorld::RemoveGoat(GameObject* o, bool andDelete) {
	playerGoats.erase(std::remove(playerGoats.begin(), playerGoats.end(), o), playerGoats.end());
	RemoveGameObject(o, andDelete);
}

void GameWorld::AddMaze(NavigationGrid* maze) {
	this->maze = maze;
}

void GameWorld::RemoveMazeNode(GameObject* node) {
	maze->RemoveNode(node->GetTransform().GetPosition());
	RemoveGameObject(node, true);
}

vector<GameObject*> GameWorld::GetGoatsInMaze() {
	vector<GameObject*> goatsInMaze;
	for (GameObject* goat : playerGoats) {
		// TODO: Somehow just get the goats in the maze
		goatsInMaze.emplace_back(goat);
	}
	return goatsInMaze;
}

vector<Vector3> GameWorld::CalculatePathInMaze(Vector3 startPos, Vector3 endPos) {
	vector<Vector3> nodes;
	NavigationPath outPath;

	bool found = maze->FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos))
		nodes.push_back(pos);

	return nodes;
}

void GameWorld::GetObjectIterators(
	GameObjectIterator& first,
	GameObjectIterator& last) const {

	first	= gameObjects.begin();
	last	= gameObjects.end();
}

void GameWorld::OperateOnContents(GameObjectFunc f) {
	for (GameObject* g : gameObjects) {
		f(g);
	}
}

void GameWorld::UpdateWorld(float dt) {
	auto rng = std::default_random_engine{};

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine e(seed);

	if (shuffleObjects) {
		std::shuffle(gameObjects.begin(), gameObjects.end(), e);
	}

	if (shuffleConstraints) {
		std::shuffle(constraints.begin(), constraints.end(), e);
	}
}

bool GameWorld::Raycast(Ray& r, RayCollision& closestCollision, bool closestObject, GameObject* ignoreThis) const {
	//The simplest raycast just goes through each object and sees if there's a collision
	RayCollision collision;

	for (auto& i : gameObjects) {
		if (!i->GetBoundingVolume()) { //objects might not be collideable etc...
			continue;
		}
		if (i == ignoreThis) {
			continue;
		}
		RayCollision thisCollision;
		if (CollisionDetection::RayIntersection(r, *i, thisCollision)) {
			
			if (!closestObject) {	
				closestCollision		= collision;
				closestCollision.node = i;
				return true;
			}
			else {
				if (thisCollision.rayDistance < collision.rayDistance) {
					thisCollision.node = i;
					collision = thisCollision;
				}
			}
		}
	}
	if (collision.node) {
		closestCollision		= collision;
		closestCollision.node	= collision.node;
		return true;
	}
	return false;
}


/*
Constraint Tutorial Stuff
*/

void GameWorld::AddConstraint(Constraint* c) {
	constraints.emplace_back(c);
}

void GameWorld::RemoveConstraint(Constraint* c, bool andDelete) {
	constraints.erase(std::remove(constraints.begin(), constraints.end(), c), constraints.end());
	if (andDelete) {
		delete c;
	}
}

void GameWorld::GetConstraintIterators(
	std::vector<Constraint*>::const_iterator& first,
	std::vector<Constraint*>::const_iterator& last) const {
	first	= constraints.begin();
	last	= constraints.end();
}