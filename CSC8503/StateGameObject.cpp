#include "StateGameObject.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"
#include "CollisionDetection.h"
#include "GameWorld.h"

using namespace NCL;
using namespace CSC8503;

StateGameObject::StateGameObject(std::string name, GameWorld* gameWorld) : GameObject(name) {
	stateMachine = new StateMachine();
	this->gameWorld = gameWorld;
}

StateGameObject::~StateGameObject() {
	delete stateMachine;
}

void StateGameObject::Update(float dt) {
	stateMachine->Update(dt);
}

/* 
	Magicballs try to stay at their starting position
	Yes I'm aware that doing the movement via different states is really stupid and can be done far simpler, but it shows basic implemention of state machines
*/
MagicBall::MagicBall(std::string name, GameWorld* gameWorld) : StateGameObject(name, gameWorld) {
	State* stateL = new State([&](float dt)-> void {
		this->MoveLeft(dt);
		});

	State* stateLF = new State([&](float dt)-> void {
		this->MoveLeft(dt);
		this->MoveForward(dt);
		});

	State* stateLB = new State([&](float dt)-> void {
		this->MoveLeft(dt);
		this->MoveBack(dt);
		});

	State* stateR = new State([&](float dt)-> void {
		this->MoveRight(dt);
		});

	State* stateRF = new State([&](float dt)-> void {
		this->MoveRight(dt);
		this->MoveForward(dt);
		});

	State* stateRB = new State([&](float dt)-> void {
		this->MoveRight(dt);
		this->MoveBack(dt);
		});

	stateMachine->AddState(stateL);
	stateMachine->AddState(stateLF);
	stateMachine->AddState(stateLB);
	stateMachine->AddState(stateR);
	stateMachine->AddState(stateRF);
	stateMachine->AddState(stateRB);

	stateMachine->AddTransition(new StateTransition(stateL, stateR, [&]()->bool {
		return GetTransform().GetPosition().x < startPos.x;
		}));

	stateMachine->AddTransition(new StateTransition(stateL, stateLF, [&]()->bool {
		return GetTransform().GetPosition().z > startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateLF, stateL, [&]()->bool {
		return GetTransform().GetPosition().z == startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateLF, stateLB, [&]()->bool {
		return GetTransform().GetPosition().z < startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateLF, stateRF, [&]()->bool {
		return GetTransform().GetPosition().x < startPos.x;
		}));

	stateMachine->AddTransition(new StateTransition(stateL, stateLB, [&]()->bool {
		return GetTransform().GetPosition().z < startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateLB, stateL, [&]()->bool {
		return GetTransform().GetPosition().z == startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateLB, stateLF, [&]()->bool {
		return GetTransform().GetPosition().z > startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateLB, stateRB, [&]()->bool {
		return GetTransform().GetPosition().x < startPos.x;
		}));

	stateMachine->AddTransition(new StateTransition(stateR, stateL, [&]()->bool {
		return GetTransform().GetPosition().x > startPos.x;
		}));

	stateMachine->AddTransition(new StateTransition(stateR, stateRF, [&]()->bool {
		return GetTransform().GetPosition().z > startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateRF, stateR, [&]()->bool {
		return GetTransform().GetPosition().z == startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateRF, stateRB, [&]()->bool {
		return GetTransform().GetPosition().z < startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateRF, stateLF, [&]()->bool {
		return GetTransform().GetPosition().x > startPos.x;
		}));

	stateMachine->AddTransition(new StateTransition(stateR, stateRB, [&]()->bool {
		return GetTransform().GetPosition().z < startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateRB, stateR, [&]()->bool {
		return GetTransform().GetPosition().z == startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateRB, stateRF, [&]()->bool {
		return GetTransform().GetPosition().z > startPos.z;
		}));

	stateMachine->AddTransition(new StateTransition(stateRB, stateLB, [&]()->bool {
		return GetTransform().GetPosition().x > startPos.x;
		}));
}

void MagicBall::MoveLeft(float dt) {
	GetPhysicsObject()->AddForce({ -10, 0, 0 });
}

void MagicBall::MoveRight(float dt) {
	GetPhysicsObject()->AddForce({ 10, 0, 0 });
}

void MagicBall::MoveForward(float dt) {
	GetPhysicsObject()->AddForce({ 0, 0, -10 });
}

void MagicBall::MoveBack(float dt) {
	GetPhysicsObject()->AddForce({ 10, 0, 10 });
}

/*
	Civilians walk around the maze, and try to run if a goat is nearby and in line of sight
*/
Civilian::Civilian(std::string name, GameWorld* gameWorld) : StateGameObject(name, gameWorld) {
	// It's easier to just halve the desired fov (40 degrees total) here instead of during every for loop
	fov = (40/2);

	// Originally every degree was checked but this was very expensive to do... checking only every 10 degrees is basically just as good and much faster!
	step = 10;
	State* walking = new State([&](float dt)->void {
		MoveForward(dt);
		});

	State* decideDirection = new State();

	State* turnLeft = new State([&](float dt)->void {
		TurnLeft(dt);
		});

	State* turnRight = new State([&](float dt)->void {
		TurnRight(dt);
		});

	State* running = new State([&](float dt)->void {
		MoveForward(dt * 4);
		});

	stateMachine->AddState(walking);
	/*
		Originally checking the optimal direction to turn in order to reach a valid facing was done every update, so it could dynamically update.
		This destroyed performance in edge cases though so it's unfortunately just done once now as an inbetween state.
		The new method does cause a few very big turns unfortunately but it works as desired in general case
	*/
	stateMachine->AddState(decideDirection);
	stateMachine->AddState(turnLeft);
	stateMachine->AddState(turnRight);
	stateMachine->AddState(running);

	StateTransitionFunction checkGoatNearby = [&, gameWorld]()->bool {
		vector<GameObject*> players = gameWorld->GetGoats();
		Vector3 civPosition = GetTransform().GetPosition();
		for (GameObject* goat : players) {
			// Check goat is close
			Vector3 goatPosition = goat->GetTransform().GetPosition();
			Vector3 positionDifference = goatPosition - civPosition;
			float distance = positionDifference.Length();
			if (distance < 50) {
				// Check if direct line of sight to goat
				Ray ray = Ray(civPosition, positionDifference.Normalised());
				RayCollision civCollision;
				if (gameWorld->Raycast(ray, civCollision, true)) {
					if (civCollision.node == goat) {
						Debug::DrawLine(civPosition, goatPosition, Vector4(1, 0, 0, 1));
						return true;
					}
					else {
						Debug::DrawLine(civPosition, goatPosition, Vector4(1, 1, 0, 1));
					}
				}
			}
		}
		return false;
	};

	stateMachine->AddTransition(new StateTransition(walking, decideDirection, [&, gameWorld]()->bool {
		Ray ray = Ray(Vector3(0, 0, 0), Vector3(0, 0, 0));
		RayCollision civCollision;
		Vector3 civPosition = GetTransform().GetPosition();
		float civDirection = GetTransform().GetOrientation().ToEuler().y;
		for (float i = step; i <= fov; i += step) {
			float dirLeft = civDirection - i;
			float dirRight = civDirection + i;
			ray = Ray(civPosition, Matrix3::FromEuler(Vector3(0, dirLeft, 0)) * Vector3(0, 0, -1));
			if (gameWorld->Raycast(ray, civCollision, true)) {
				if (civCollision.rayDistance < 10) {
					Debug::DrawLine(civPosition, civCollision.collidedAt, Vector4(1, 0, 0, 1));
					return true;
				}
				else {
					Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 10, Vector4(0, 0, 1, 1));
					Debug::DrawLine(civPosition + ray.GetDirection() * 10, civCollision.collidedAt, Vector4(0, 1, 0, 1));
				}
			}
			else {
				Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 10, Vector4(0, 0, 1, 1));
			}
			ray = Ray(civPosition, Matrix3::FromEuler(Vector3(0, dirRight, 0)) * Vector3(0, 0, -1));
			if (gameWorld->Raycast(ray, civCollision, true)) {
				if (civCollision.rayDistance < 10) {
					Debug::DrawLine(civPosition, civCollision.collidedAt, Vector4(1, 0, 0, 1));
					return true;
				}
				else {
					Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 10, Vector4(0, 0, 1, 1));
					Debug::DrawLine(civPosition + ray.GetDirection() * 10, civCollision.collidedAt, Vector4(0, 1, 0, 1));
				}
			}
			else {
				Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 10, Vector4(0, 0, 1, 1));
			}
		}
		return false;
		}));

	stateMachine->AddTransition(new StateTransition(walking, running, checkGoatNearby));

	stateMachine->AddTransition(new StateTransition(decideDirection, turnRight, [&, gameWorld]()->bool {
		Ray ray = Ray(Vector3(0, 0, 0), Vector3(0, 0, 0));
		RayCollision civCollision;
		Vector3 civPosition = GetTransform().GetPosition();
		float civDirection = GetTransform().GetOrientation().ToEuler().y;
		for (float i = fov; i < 180; i += 1) { // Keep checking both directions, if it turns out right is closer to a success then instead turn right
			float dirLeft = civDirection - i;
			float dirRight = civDirection + i;
			
			ray = Ray(civPosition, Matrix3::FromEuler(Vector3(0, dirLeft, 0)) * Vector3(0, 0, -1));
			Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 15, Vector4(1, 0, 1, 1));
			if (gameWorld->Raycast(ray, civCollision, true))
				if (civCollision.rayDistance > 15)
					return true;
			ray = Ray(civPosition, Matrix3::FromEuler(Vector3(0, dirRight, 0)) * Vector3(0, 0, -1));
			Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 15, Vector4(1, 0, 1, 1));
			if (gameWorld->Raycast(ray, civCollision, true))
				if (civCollision.rayDistance > 15)
					return false;
		}
		return false;
		}));

	stateMachine->AddTransition(new StateTransition(decideDirection, turnLeft, [&, gameWorld]()->bool {
		Ray ray = Ray(Vector3(0, 0, 0), Vector3(0, 0, 0));
		RayCollision civCollision;
		Vector3 civPosition = GetTransform().GetPosition();
		float civDirection = GetTransform().GetOrientation().ToEuler().y;
		for (float i = fov; i < 180; i += 1) { // Keep checking both directions, if it turns out left is closer to a success then instead turn left
			float dirLeft = civDirection - i;
			float dirRight = civDirection + i;

			ray = Ray(civPosition, Matrix3::FromEuler(Vector3(0, dirRight, 0)) * Vector3(0, 0, -1));
			Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 15, Vector4(1, 0, 1, 1));
			if (gameWorld->Raycast(ray, civCollision, true))
				if (civCollision.rayDistance > 15)
					return true;
			ray = Ray(civPosition, Matrix3::FromEuler(Vector3(0, dirLeft, 0)) * Vector3(0, 0, -1));
			Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 15, Vector4(1, 0, 1, 1));
			if (gameWorld->Raycast(ray, civCollision, true))
				if (civCollision.rayDistance > 15)
					return false;
		}
		return false;
		}));

	stateMachine->AddTransition(new StateTransition(turnLeft, walking, [&, gameWorld]()->bool {
		Ray ray = Ray(Vector3(0, 0, 0), Vector3(0, 0, 0));
		RayCollision civCollision;
		Vector3 civPosition = GetTransform().GetPosition();
		float civDirection = GetTransform().GetOrientation().ToEuler().y;
		bool ret = true;
		for (float i = -fov; i <= fov; i += step) {
			float dir = civDirection + i;
			if (dir < -180)
				dir += 360;
			else if (dir > 180)
				dir -= 360;
			ray = Ray(civPosition, Matrix3::FromEuler(Vector3(0, dir, 0)) * Vector3(0, 0, -1));
			if (gameWorld->Raycast(ray, civCollision, true)) {
				if (civCollision.rayDistance < 10) {
					Debug::DrawLine(civPosition, civCollision.collidedAt, Vector4(1, 0, 0, 1));
					/* 
						Originally thisand the other turnLeft / right states showed all the collision rays as it was more interesting, but this tanked performance unfortunately.
						The old method is still here, delete return = false if you want to see it all, but this will greatly reduce performance, maybe reduce to 1-3 civilians in InitWorld first?
					*/
					return false;
					ret = false;
				}
				else {
					Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 10, Vector4(0, 0, 1, 1));
					Debug::DrawLine(civPosition + ray.GetDirection() * 10, civCollision.collidedAt, Vector4(0, 1, 0, 1));
				}
			}
			else {
				Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 10, Vector4(0, 0, 1, 1));
			}
		}
		return ret;
		}));

	stateMachine->AddTransition(new StateTransition(turnLeft, running, checkGoatNearby));

	stateMachine->AddTransition(new StateTransition(turnRight, walking, [&, gameWorld]()->bool {
		Ray ray = Ray(Vector3(0, 0, 0), Vector3(0, 0, 0));
		RayCollision civCollision;
		Vector3 civPosition = GetTransform().GetPosition();
		float civDirection = GetTransform().GetOrientation().ToEuler().y;
		bool ret = true;
		for (float i = fov; i >= -fov; i -= step) {
			float dir = civDirection + i;
			if (dir < -180)
				dir += 360;
			else if (dir > 180)
				dir -= 360;
			ray = Ray(civPosition, Matrix3::FromEuler(Vector3(0, dir, 0)) * Vector3(0, 0, -1));
			if (gameWorld->Raycast(ray, civCollision, true)) {
				if (civCollision.rayDistance < 10) {
					Debug::DrawLine(GetTransform().GetPosition(), civCollision.collidedAt, Vector4(1, 0, 0, 1));
					return false;
					ret = false;
				}
				else {
					Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 10, Vector4(0, 0, 1, 1));
					Debug::DrawLine(civPosition + ray.GetDirection() * 10, civCollision.collidedAt, Vector4(0, 1, 0, 1));
				}
			}
			else {
				Debug::DrawLine(civPosition, civPosition + ray.GetDirection() * 10, Vector4(0, 0, 1, 1));
			}
		}
		return ret;
		}));

	stateMachine->AddTransition(new StateTransition(turnRight, running, checkGoatNearby));

	stateMachine->AddTransition(new StateTransition(running, walking, [&, gameWorld]()->bool {
		vector<GameObject*> players = gameWorld->GetGoats();
		Vector3 civPosition = GetTransform().GetPosition();
		for (GameObject* goat : players) {
			// Check goat is close
			Vector3 goatPosition = goat->GetTransform().GetPosition();
			Vector3 positionDifference = goatPosition - civPosition;
			float distance = positionDifference.Length();
			if (distance < 50) {
				// Check if direct line of sight to goat
				Ray ray = Ray(civPosition, positionDifference.Normalised());
				RayCollision civCollision;
				if (gameWorld->Raycast(ray, civCollision, true)) {
					if (civCollision.node == goat) {
						Debug::DrawLine(civPosition, goatPosition, Vector4(1, 0, 0, 1));
						return false;
					}
					else {
						Debug::DrawLine(civPosition, goatPosition, Vector4(1, 1, 0, 1));
					}
				}
			}
		}
		return true;
		}));
}

void Civilian::MoveForward(float dt) {
	Matrix3 forceRotation = Matrix3::FromEuler(Vector3(0, GetTransform().GetOrientation().ToEuler().y, 0));
	GetPhysicsObject()->AddForce(forceRotation * Vector3(0, 0, -15 * dt));
}

void Civilian::TurnLeft(float dt) {
	GetPhysicsObject()->AddTorque(Vector3(0, 5 * dt, 0));
}

void Civilian::TurnRight(float dt) {
	GetPhysicsObject()->AddTorque(Vector3(0, -5 * dt, 0));
}