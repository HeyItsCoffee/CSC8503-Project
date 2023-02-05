#include "Window.h"

#include "Debug.h"

#include "StateMachine.h"
#include "StateTransition.h"
#include "State.h"

#include "GameServer.h"
#include "GameClient.h"

#include "NavigationGrid.h"
#include "NavigationMesh.h"

#include "TutorialGame.h"
#include "NetworkedGame.h"

#include "PushdownMachine.h"

#include "PushdownState.h"

#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"
#include "BehaviourParallel.h"

using namespace NCL;
using namespace CSC8503;

#include <chrono>
#include <thread>
#include <sstream>
#include <regex>

int option;
int a = 127;
int b = 0;
int c = 0;
int d = 1;

string IPAsString(int a, int b, int c, int d) {
	return std::to_string(a) + "." + std::to_string(b) + "." + std::to_string(c) + "." + std::to_string(d);
}

class EnterIPScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		std::cout << "Enter server IP address in format a.b.c.d: ";
		string input, value;
		std::cin >> input;
		if (std::regex_match(input, std::regex("(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"))) {
			a = std::stoi(input.substr(0, input.find(".")));
			input.erase(0, input.find(".") + 1);
			b = std::stoi(input.substr(0, input.find(".")));
			input.erase(0, input.find(".") + 1);
			c = std::stoi(input.substr(0, input.find(".")));
			input.erase(0, input.find(".") + 1);
			d = std::stoi(input.substr(0, input.find(".")));
			return PushdownResult::Pop;
		}
		std::cout << "Invalid input\n";
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		std::cout << "Server is currently set to " + IPAsString(a, b, c, d) << std::endl;
	}
protected:
	int coinsMined = 0;
	float pauseReminder = 1;
};

class IntroScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		std::cout << "Choose a game mode\n1) Start singleplayer game\n2) Start multiplayer server\n3) Start multiplayer client (Server IP: " + IPAsString(a, b, c, d) + ")\n4) Set IP of server to connect to\n5) Quit\nChoose a game mode : ";
		string input;
		std::cin >> input;
		if (std::regex_match(input, std::regex("[1-5]"))) {
			switch (input[0]) {
			case('1'):
				option = 1;
				return PushdownResult::Pop;
			case('2'):
				option = 2;
				return PushdownResult::Pop;
			case('3'):
				option = 3;
				return PushdownResult::Pop;
			case('4'):
				*newState = new EnterIPScreen();
				return PushdownResult::Push;
			case('5'):
				option = 5;
				return PushdownResult::Pop;
			}
		}
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		std::cout << "Welcome to Goat Imitator 2022!\n";
		std::cout << "Press F1\n";
	}
};

void StartConsoleMenu() {
	PushdownMachine machine(new IntroScreen());
	while (machine.Update(0)) {}
}

class TestPacketReceiver : public PacketReceiver {
public:
	TestPacketReceiver(string name) {
		this->name = name;
	}

	void ReceivePacket(int type, GamePacket* payload, int source) {
		if (type == String_Message) {
			StringPacket* realPacket = (StringPacket*)payload;

			string msg = realPacket->GetStringFromData();

			std::cout << name << " received message: " << msg << std::endl;
		}
	}

protected:
	string name;
};

/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead. 

This time, we've added some extra functionality to the window class - we can
hide or show the 

*/
int main() {
	StartConsoleMenu();
	TutorialGame* g;
	Window* w = Window::CreateGameWindow("CSC8503 Game technology!", 1280, 720);

	if (!w->HasInitialised()) {
		return -1;
	}

	switch (option) {
	case(1):
		g = new TutorialGame(true);
		break;
	case(2):
		std::cout << "I will be server!";
		g = new NetworkedGame();
		break;
	case(3):
		std::cout << "I will be client connecting to IP " << IPAsString(a, b, c, d) << "!";
		g = new NetworkedGame(a, b, c, d);
		break;
	default:
		return 0;
	}

	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);

	w->GetTimer()->GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!
	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		float dt = w->GetTimer()->GetTimeDeltaSeconds();
		if(dt > 1.0f){
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
			w->ShowConsole(false);
		}

		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Y)) {
			w->SetWindowPosition(0, 0);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::U)) {
			w->SetWindowPosition(-1920, 0);
		}

		//std::cout << "LAST FRAME: " << frames[0] << " AVERAGE: " << avg << "/" << 1000.0f/avg << "fps" << std::endl;

		w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt) + " / " + std::to_string(1.0f / dt) + "fps");

		g->UpdateGame(dt);
	}
	Window::DestroyGameWindow();
}