#include "TutorialGame.h"
#include "GameWorld.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"

#include "PositionConstraint.h"
#include "OrientationConstraint.h"
#include "StateGameObject.h"
#include "Goat.h"
#include <fstream>

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame(bool local) : local(local){
	world		= new GameWorld();
#ifdef USEVULKAN
	renderer	= new GameTechVulkanRenderer(*world);
#else 
	renderer = new GameTechRenderer(*world);
#endif

	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= true;
	inSelectionMode = false;
	goatControlMode = true;

	if (local) {
		string line;
		try {
			std::ifstream file("HighScoreLocal.txt");
			while (getline(file, line)) {
				highScore = std::stoi(line);
			}
			file.close();
		}
		catch(std::exception ex){
			highScore = 0;
		}
	}
	else {
		highScore = 0;
	}

	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	cubeMesh	= renderer->LoadMesh("cube.msh");
	sphereMesh	= renderer->LoadMesh("sphere.msh");
	charMesh	= renderer->LoadMesh("goat.msh");
	enemyMesh	= renderer->LoadMesh("Keeper.msh");
	bonusMesh	= renderer->LoadMesh("apple.msh");
	capsuleMesh = renderer->LoadMesh("capsule.msh");
	gooseMesh	= renderer->LoadMesh("goose.msh");

	basicTex	= renderer->LoadTexture("checkerboard.png");
	basicShader = renderer->LoadShader("scene.vert", "scene.frag");

	dogeTex = renderer->LoadTexture("Default.png");
	grassTex = renderer->LoadTexture("green_lush_grass_textures_free_23-1923293337.png");
	hedgeTex = renderer->LoadTexture("Corn.png");

	InitCamera();
	InitWorld(local);
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete charMesh;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete dogeTex;
	delete hedgeTex;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateGame(float dt) {
	if (local) {
		gameDurationRemaining -= dt;
		if (gameDurationRemaining < 0) {
			if (finalScore == -1) {
				finalScore = goat->GetScore();
			}
			if (finalScore != 0) {
				Debug::Print("Final Score: " + std::to_string(finalScore), Vector2(0, 20));
				if (finalScore > highScore) {
					Debug::Print("NEW HIGH SCORE", Vector2(0, 25));
				}
			}
			else
				Debug::Print("You didn't hit anybody, you lose!", Vector2(0, 20));
		}
		if (gameDurationRemaining < -5) {
			if (highScore < finalScore) {
				highScore = finalScore;
				string line;
				try {
					std::ofstream ofile("HighScoreLocal.txt");
					ofile << highScore;
					ofile.flush();
					ofile.close();
				}
				catch (std::exception ex) { }
			}
			InitWorld(true);
		}
		if (!inSelectionMode) {
			if (goatControlMode)
				world->GetMainCamera()->UpdateCamera(dt, goat->GetTransform().GetPosition(), goat->GetTransform().GetScale());
			else world->GetMainCamera()->UpdateCamera(dt);
		}
		if (lockedObject != nullptr) {
			Vector3 objPos = lockedObject->GetTransform().GetPosition();
			Vector3 camPos = objPos + lockedOffset;

			Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

			Matrix4 modelMat = temp.Inverse();

			Quaternion q(modelMat);
			Vector3 angles = q.ToEuler(); //nearly there now!

			world->GetMainCamera()->SetPosition(camPos);
			world->GetMainCamera()->SetPitch(angles.x);
			world->GetMainCamera()->SetYaw(angles.y);
		}

		UpdateKeys();

		if (useGravity) {
			Debug::Print("(G)ravity on", Vector2(5, 95), Debug::RED);
		}
		else {
			Debug::Print("(G)ravity off", Vector2(5, 95), Debug::RED);
		}

		RayCollision closestCollision;
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::K) && selectionObject) {
			Vector3 rayPos;
			Vector3 rayDir;

			rayDir = selectionObject->GetTransform().GetOrientation() * Vector3(0, 0, -1);

			rayPos = selectionObject->GetTransform().GetPosition();

			Ray r = Ray(rayPos, rayDir);

			if (world->Raycast(r, closestCollision, true, selectionObject)) {
				if (objClosest) {
					objClosest->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				}
				objClosest = (GameObject*)closestCollision.node;

				objClosest->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));
			}
		}

		Debug::DrawLine(Vector3(), Vector3(100, 0, 0), Vector4(0, 1, 0, 1));
		Debug::DrawLine(Vector3(), Vector3(0, 100, 0), Vector4(1, 0, 0, 1));
		Debug::DrawLine(Vector3(), Vector3(0, 0, 100), Vector4(0, 0, 1, 1));

		if (goatControlMode) {
			Debug::Print("Press F3 to change to creative mode!", Vector2(5, 85));
			GoatControl(dt, goat, (int)world->GetMainCamera()->GetYaw(),
				Window::GetKeyboard()->KeyDown(KeyboardKeys::UP), Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN),
				Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT), Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT));
		}
		else if (inSelectionMode) {
			Debug::Print("Press Q to change to camera mode!", Vector2(5, 85));
			SelectObject();
			MoveSelectedObject();
		}
		else {
			Debug::Print("Press Q to change to selection mode!", Vector2(5, 85));
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::T) && selectionObject) {
			world->RemoveMazeNode(selectionObject);
			selectionObject = nullptr;
		}
		int minsLeft = gameDurationRemaining / 60;
		int secondsLeft = gameDurationRemaining - (60 * minsLeft);
		Debug::Print("Time Remaining: " + std::to_string(minsLeft) + ':' + (secondsLeft < 10 ? "0" : "") + std::to_string(secondsLeft), Vector2(0, 10));
		Debug::Print("Civilians Remaining: " + std::to_string((stateGameObjects.size() - 5 - goat->GetScore())), Vector2(0, 15));
	}
	else if (goat) {
		world->GetMainCamera()->UpdateCamera(dt, goat->GetTransform().GetPosition(), goat->GetTransform().GetScale());
		GoatControl(dt, goat, (int)world->GetMainCamera()->GetYaw(),
			Window::GetKeyboard()->KeyDown(KeyboardKeys::UP), Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN),
			Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT), Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT));
		int minsLeft = gameDurationRemaining / 60;
		int secondsLeft = gameDurationRemaining - (60 * minsLeft);
		Debug::Print("Time Remaining: " + std::to_string(minsLeft) + ':' + (secondsLeft < 10 ? "0" : "") + std::to_string(secondsLeft), Vector2(0, 10));
		Debug::Print("Civilians Remaining: " + std::to_string((stateGameObjects.size() - 5 - goat->GetScore())), Vector2(0, 15));
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F6)) {
		gameDurationRemaining = 5;
	}
	
	for (StateGameObject* i : stateGameObjects)
		i->Update(dt);

	for (BehaviourTreeObject* i : behaviourTreeObjects)
		i->Execute(dt);

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	renderer->Render();
	Debug::UpdateRenderables(dt);
}


void TutorialGame::GoatControl(float dt, Goat* goat, int cameraYaw, bool up, bool down, bool left, bool right) {
	// Rotate goat based on camera orientation
	Vector3 goatRotation = goat->GetTransform().GetOrientation().ToEuler();
	float yaw = goatRotation.y;
	if (yaw < 0)
		yaw += 360;
	float yawDiff = cameraYaw - yaw;
	if (yawDiff > 180)
		yawDiff -= 360;
	else if (yawDiff < -180)
		yawDiff += 360;

	goat->GetPhysicsObject()->AddTorque(Vector3(0, sqrt(abs(yawDiff)) * (yawDiff < 0 ? -1 : 1) * 10 * dt, 0));
	Matrix3 forceRotation = Matrix3::FromEuler(Vector3(0, (float)cameraYaw, 0));

	float forceAmount = 100 * dt;

	// Move goat based on arrow key input
	if (up) {
		goat->GetPhysicsObject()->AddForce(forceRotation * Vector3(0, 0, -forceAmount));
	}
	else if (down) {
		goat->GetPhysicsObject()->AddForce(forceRotation * Vector3(0, 0, forceAmount));
	}
	if (left) {
		goat->GetPhysicsObject()->AddForce(forceRotation * Vector3(-forceAmount, 0, 0));
	}
	else if (right) {
		goat->GetPhysicsObject()->AddForce(forceRotation * Vector3(forceAmount, 0, 0));
	}
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(true); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F3)) {
		goatControlMode = !goatControlMode; // By default control the goat, F3 returns to 'creative mode'
		inSelectionMode = !goatControlMode;
		if (goatControlMode) {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
		else {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		if (!goatControlMode) {
			inSelectionMode = !inSelectionMode;
			if (inSelectionMode) {
				Window::GetWindow()->ShowOSPointer(true);
				Window::GetWindow()->LockMouseToWindow(false);
			}
			else {
				Window::GetWindow()->ShowOSPointer(false);
				Window::GetWindow()->LockMouseToWindow(true);
			}
		}
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();


	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		selectionObject->GetPhysicsObject()->AddForce(fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		selectionObject->GetPhysicsObject()->AddForce(-fwdAxis);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NEXT)) {
		selectionObject->GetPhysicsObject()->AddForce(Vector3(0,-10,0));
	}
}

void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}
}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(2500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void TutorialGame::InitWorld(bool local) {
	world->ClearAndErase();
	srand(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	physics->Clear();
	stateGameObjects.clear();
	behaviourTreeObjects.clear();

	AddGrassFloor();
	AddMazeToWorld("CornMaze.txt", Vector3(-175, -20, -175));
	AddRandomSpheres();
	AddCiviliansToWorld(10);
	AddMagicBallsToWorld(5);
	AddBridgeToWorld(Vector3(10, -8, -175), Vector3(30, -8, -175));
	AddBridgeToWorld(Vector3(10, -8, 185), Vector3(30, -8, 185));
	AddGooseToWorld(Vector3(50, -10, 50));
	if (local) {
		goat = AddPlayerToWorld();
	}
	gameDurationRemaining = 120.0f;
	finalScore = -1;
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position, TextureBase* texture, Vector3 floorSize) {
	GameObject* floor = new GameObject("Floor");
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, texture, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume(), true));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

GameObject* TutorialGame::AddWallToWorld(const Vector3& position, float height, float width, bool alongX) {
	GameObject* wall = new GameObject("Wall");

	Vector3 floorSize = Vector3((alongX ? width : 0), height, (!alongX ? width : 0));
	AABBVolume* volume = new AABBVolume(floorSize);
	wall->SetBoundingVolume((CollisionVolume*)volume);
	wall->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(Vector3(position.x, position.y + height, position.z));

	wall->SetRenderObject(new RenderObject(&wall->GetTransform(), cubeMesh, basicTex, basicShader));
	wall->SetPhysicsObject(new PhysicsObject(&wall->GetTransform(), wall->GetBoundingVolume(), true));

	wall->GetPhysicsObject()->SetInverseMass(0);
	wall->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall);

	return wall;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass, string name) {
	GameObject* sphere = new GameObject(name);

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass, string name) {
	GameObject* cube = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, dogeTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddHedgeToWorld(const Vector3& position, Vector3 dimensions, string name) {
	GameObject* cube = new GameObject(name);

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, hedgeTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(0);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

Goat* TutorialGame::AddPlayerToWorld() {
	float meshSize		= 2.0f;
	float inverseMass	= 3.0f;

	Vector3 startPoint = world->GetMaze()->GetZeroPos();
	Vector3 endPoint = startPoint + (Vector3(1.0f, 0, 0) * (float)world->GetMaze()->GetWidth() * (float)world->GetMaze()->GetNodeSize())
		+ (Vector3(0, 0, 1.0f) * (float)world->GetMaze()->GetHeight() * (float)world->GetMaze()->GetNodeSize());
	Vector3 deltaPoints = endPoint - startPoint;
	Vector3 randPos;
	SphereVolume* volume = new SphereVolume(meshSize);
	for (int i = 0; i < 1; i++) {
		randPos = Vector3((rand() % (int)deltaPoints.x) + startPoint.x, startPoint.y + rand() % 5 + 15, (rand() % (int)deltaPoints.z) + startPoint.z);
		if (!world->GetMaze()->FullyWithinOpenNodes(randPos, Vector3(volume->GetRadius() * 0.5, volume->GetRadius() * 0.5, volume->GetRadius() * 0.5))) {
			i--;
		}
	}

	Goat* character = new Goat("Player");

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(randPos);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), charMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->setCRest(0.5f);
	character->setCFric(0.5f);

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();
	character->GetPhysicsObject()->SetAxisLock(true, false, true);

	world->AddGoat(character);

	return character;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject("Enemy");

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	GameObject* apple = new GameObject("Bonus");

	SphereVolume* volume = new SphereVolume(0.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

StateGameObject* TutorialGame::AddCivilianToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 4.0f;

	StateGameObject* civilian = new Civilian("Civilian", world);

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	civilian->SetBoundingVolume((CollisionVolume*)volume);

	civilian->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	civilian->SetRenderObject(new RenderObject(&civilian->GetTransform(), enemyMesh, nullptr, basicShader));
	civilian->SetPhysicsObject(new PhysicsObject(&civilian->GetTransform(), civilian->GetBoundingVolume()));

	civilian->GetPhysicsObject()->SetInverseMass(inverseMass);
	civilian->GetPhysicsObject()->InitSphereInertia();

	civilian->setCRest(0.5f);
	civilian->setCFric(0.5f);

	civilian->GetTransform().SetOrientation(Quaternion::EulerAnglesToQuaternion(0, rand() % 360, 0));

	world->AddGameObject(civilian);

	stateGameObjects.push_back(civilian);

	return civilian;
}

void TutorialGame::AddCiviliansToWorld(int count) {
	Vector3 startPoint = world->GetMaze()->GetZeroPos();
	Vector3 endPoint = startPoint + (Vector3(1.0f, 0, 0) * (float)world->GetMaze()->GetWidth() * (float)world->GetMaze()->GetNodeSize())
		+ (Vector3(0, 0, 1.0f) * (float)world->GetMaze()->GetHeight() * (float)world->GetMaze()->GetNodeSize());
	Vector3 deltaPoints = endPoint - startPoint;
	AABBVolume volume = AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * 3.0f);
	for (int i = 0; i < count; i++) {
		Vector3 randPos = Vector3((rand() % (int)deltaPoints.x) + startPoint.x, startPoint.y + rand() % 5 + 15, (rand() % (int)deltaPoints.z) + startPoint.z);
		if (!world->GetMaze()->FullyWithinOpenNodes(randPos, volume.GetHalfDimensions())) {
			i--;
		}
		else {
			AddCivilianToWorld(randPos);
		}
	}
}

StateGameObject* TutorialGame::AddMagicBallToWorld(const Vector3& position, float radius, float inverseMass) {
	MagicBall* magicBall = new MagicBall("MagicBall", world);

	Vector3 magicBallSize = Vector3(radius, radius, radius);
	SphereVolume* ballVolume = new SphereVolume(radius);
	magicBall->SetBoundingVolume((CollisionVolume*)ballVolume);

	magicBall->GetTransform()
		.SetScale(magicBallSize)
		.SetPosition(position);

	magicBall->SetStartPos(position);

	magicBall->SetRenderObject(new RenderObject(&magicBall->GetTransform(), sphereMesh, basicTex, basicShader));
	magicBall->GetRenderObject()->SetColour(Vector4(1, 1, 0, 1));
	magicBall->SetPhysicsObject(new PhysicsObject(&magicBall->GetTransform(), magicBall->GetBoundingVolume()));

	magicBall->GetPhysicsObject()->SetInverseMass(inverseMass);
	magicBall->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(magicBall);

	stateGameObjects.push_back(magicBall);
	return magicBall;
}

void TutorialGame::AddMagicBallsToWorld(int amount) {
	float radius = 4.0f;
	Vector3 startPoint = world->GetMaze()->GetZeroPos();
	Vector3 endPoint = startPoint + (Vector3(1.0f, 0, 0) * (float)world->GetMaze()->GetWidth() * (float)world->GetMaze()->GetNodeSize())
		+ (Vector3(0, 0, 1.0f) * (float)world->GetMaze()->GetHeight() * (float)world->GetMaze()->GetNodeSize());
	Vector3 deltaPoints = endPoint - startPoint;
	SphereVolume* volume = new SphereVolume(radius);
	for (int i = 0; i < amount; i++) {
		Vector3 randPos = Vector3((rand() % (int)deltaPoints.x) + startPoint.x, startPoint.y + rand() % 5 + 15, (rand() % (int)deltaPoints.z) + startPoint.z);
		if (!world->GetMaze()->FullyWithinOpenNodes(randPos, Vector3(volume->GetRadius() * 0.5, volume->GetRadius() * 0.5, volume->GetRadius() * 0.5))) {
			i--;
		}
		else {
			AddMagicBallToWorld(randPos, radius, 4.0f);
		}
	}
}

BehaviourTreeObject* TutorialGame::AddGooseToWorld(const Vector3& position) {
	float meshSize = 1.5f;
	float inverseMass = 1.5f;

	BehaviourTreeObject* goose = new Goose(world);

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	goose->SetBoundingVolume((CollisionVolume*)volume);

	goose->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	goose->SetRenderObject(new RenderObject(&goose->GetTransform(), gooseMesh, nullptr, basicShader));
	goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));

	goose->GetPhysicsObject()->SetInverseMass(inverseMass);
	goose->GetPhysicsObject()->InitSphereInertia();

	goose->setCRest(0.5f);
	goose->setCFric(0.5f);

	world->AddGameObject(goose);

	behaviourTreeObjects.push_back(goose);

	return goose;
}

void TutorialGame::InitWalls() {
	AddWallToWorld(Vector3(0, -20, 50), 10, 40, true);
	AddWallToWorld(Vector3(0, -20, -50), 10, 40, true);
	AddWallToWorld(Vector3(50, -20, 0), 10, 40, false);
	AddWallToWorld(Vector3(-50, -20, 0), 10, 40, false);
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -20, 0), basicTex);
}

void TutorialGame::AddGrassFloor() {
	AddFloorToWorld(Vector3(0, -20, 0), grassTex);
}

void TutorialGame::InitGameExamples() {
	AddPlayerToWorld();
	AddEnemyToWorld(Vector3(5, 5, 0));
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0), basicTex);
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, int numIsles, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);
	float isleSpacing = 10.0f;

	int counter = 0;

	for(int y = 0; y < numIsles; y++)
		for (int x = 0; x < numCols; ++x) 
			for (int z = 0; z < numRows; ++z) {
				Vector3 position = Vector3(x * colSpacing, (y+1)*isleSpacing, z * rowSpacing);

				if (rand() % 2) {
					AddCubeToWorld(position, cubeDims, 10.0f, "Cube " + std::to_string(counter));
				}
				else {
					AddSphereToWorld(position, sphereRadius, 10.0f, "Sphere " + std::to_string(counter));
				}

				counter++;
			}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}

void TutorialGame::AddMazeToWorld(string fileName, Vector3 zeroPos) {
	NavigationGrid* maze = new NavigationGrid(fileName, zeroPos);
	int nodeSize = maze->GetNodeSize();
	int height = maze->GetHeight();
	int width = maze->GetWidth();

	vector<GridNode> allNodes = maze->GetNodes();

	for (int z = 0; z < height; z++) {
		for (int x = 0; x < width; x++) {
			GridNode& n = allNodes[(width * z) + x];
			if(n.type == 'x')
				AddHedgeToWorld(n.position + Vector3(0, 10.0f, 0), Vector3((float)nodeSize/2, 10.0f, (float)nodeSize/2), "Maze Node " + (width * z) + x);
		}
	}
	world->AddMaze(maze);
}

void TutorialGame::AddRandomSpheres() {
	Vector3 startPoint = world->GetMaze()->GetZeroPos();
	Vector3 endPoint = startPoint + (Vector3(1.0f, 0, 0) * (float)world->GetMaze()->GetWidth() * (float)world->GetMaze()->GetNodeSize())
		+ (Vector3(0, 0, 1.0f) * (float)world->GetMaze()->GetHeight() * (float)world->GetMaze()->GetNodeSize());
	Vector3 deltaPoints = endPoint - startPoint;
	for (int i = 0; i < 100; i++) {
		Vector3 randPos = Vector3((rand() % (int)deltaPoints.x) + startPoint.x, startPoint.y + rand() % 5 + 5, (rand() % (int)deltaPoints.z) + startPoint.z);
		GameObject* sphere = AddSphereToWorld(randPos, 3.0f, 2.0f, "Random Ball");
		Vector3 aabb;
		sphere->GetBroadphaseAABB(aabb);
		if (!world->GetMaze()->FullyWithinOpenNodes(randPos, aabb)) {
			world->RemoveGameObject(sphere, true);
			i--;
		}
	}
}

void TutorialGame::AddBridgeToWorld(Vector3 from, Vector3 to) {
	int numLinks = sqrt((from - to).Length());
	float lengthSize = (from - to).Length() / (numLinks * 7.5f);
	Vector3 cubeSize = Vector3(lengthSize, lengthSize, lengthSize);

	float invCubeMass = 1.0f; // How heavy middle pieces are
	float maxDistance = (to - from).Length() / (numLinks * 3) * 3; // Constraint distance
	float cubeDistance = (to - from).Length() / (numLinks * 3);

	GameObject* start = AddCubeToWorld(from + Vector3(0, 0, 0), cubeSize, 0);
	start->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
	GameObject* end = AddCubeToWorld(to, cubeSize, 0);
	end->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));

	GameObject* previous = start;

	for (int i = 0; i < numLinks; i++) {
		GameObject* block = AddCubeToWorld(from + (to - from).Normalised() * cubeDistance * i, cubeSize, invCubeMass);
		block->GetRenderObject()->SetColour(Vector4(1.0f/numLinks*i,0,0,1));
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}
	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

/*
Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
		if (selectionObject) {	//set colour to deselected;
			selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
			selectionObject = nullptr;
		}

		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			selectionObject = (GameObject*)closestCollision.node;

			selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));

			//Debug::DrawLine(ray.GetPosition() - Vector3(0, 0.1f, 0), closestCollision.collidedAt, Vector4(1, 1, 0, 1), 10.0f);

			bool sleep = selectionObject->GetPhysicsObject()->GetSleepState();
			if (sleep)
				std::cout << "SLEEPING\n";
			else
				std::cout << "NOT SLEEPING\n";

			Ray objRay = CollisionDetection::BuildRayFromObjectFacing(*selectionObject);

			RayCollision targetCollision;
			if (world->Raycast(objRay, targetCollision, true, selectionObject)) 
				Debug::DrawLine(objRay.GetPosition(), targetCollision.collidedAt, Vector4(0, 1, 1, 1), 10.0f);
			else
				Debug::DrawLine(objRay.GetPosition(), objRay.GetPosition() + objRay.GetDirection()*25, Vector4(1, 0, 1, 1), 10.0f);

			return true;
		}
		else {
			return false;
		}
	}
	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject) {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	Debug::Print("Click Force:" + std::to_string(forceMagnitude), Vector2(5, 90));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return;//we haven't selected anything!
	}
	//Push the selected object!
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}


