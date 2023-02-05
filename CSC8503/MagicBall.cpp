#include "StateGameObject.h"
#include "MagicBall.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PhysicsObject.h"

using namespace NCL;
using namespace CSC8503;

MagicBall::MagicBall(std::string name) {
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
		return GetTransform().GetPosition().x < 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateL, stateLF, [&]()->bool {
		return GetTransform().GetPosition().z > 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateLF, stateL, [&]()->bool {
		return GetTransform().GetPosition().z == 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateLF, stateLB, [&]()->bool {
		return GetTransform().GetPosition().z < 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateLF, stateRF, [&]()->bool {
		return GetTransform().GetPosition().x < 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateL, stateLB, [&]()->bool {
		return GetTransform().GetPosition().z < 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateLB, stateL, [&]()->bool {
		return GetTransform().GetPosition().z == 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateLB, stateLF, [&]()->bool {
		return GetTransform().GetPosition().z > 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateLB, stateRB, [&]()->bool {
		return GetTransform().GetPosition().x < 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateR, stateL, [&]()->bool {
		return GetTransform().GetPosition().x > 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateR, stateRF, [&]()->bool {
		return GetTransform().GetPosition().z > 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateRF, stateR, [&]()->bool {
		return GetTransform().GetPosition().z == 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateRF, stateRB, [&]()->bool {
		return GetTransform().GetPosition().z < 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateRF, stateLF, [&]()->bool {
		return GetTransform().GetPosition().x > 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateR, stateRB, [&]()->bool {
		return GetTransform().GetPosition().z < 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateRB, stateR, [&]()->bool {
		return GetTransform().GetPosition().z == 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateRB, stateRF, [&]()->bool {
		return GetTransform().GetPosition().z > 0;
		}));

	stateMachine->AddTransition(new StateTransition(stateRB, stateLB, [&]()->bool {
		return GetTransform().GetPosition().x > 0;
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