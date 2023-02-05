#include "NetworkedGame.h"
#include "NetworkPlayer.h"
#include "NetworkObject.h"
#include "GameServer.h"
#include "GameClient.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "Goat.h"

#include <fstream>

#define COLLISION_MSG 30

struct MessagePacket : public GamePacket {
	short messageID;
	short messageValue;
	short messageValueTwo;
	short highScore;

	MessagePacket() {
		type = Message;
		size = sizeof(short) * 4;
	}
};

NetworkedGame::NetworkedGame() : TutorialGame(false)	{
	thisServer = nullptr;
	thisClient = nullptr;

	NetworkBase::Initialise();
	timeToNextPacket  = 0.0f;
	packetsToSnapshot = 0;
	StartAsServer();
}

NetworkedGame::NetworkedGame(int a, int b, int c, int d) : TutorialGame(false) {
	thisServer = nullptr;
	thisClient = nullptr;

	NetworkBase::Initialise();
	timeToNextPacket = 0.0f;
	packetsToSnapshot = 0;
	StartAsClient(a, b, c, d);
}

NetworkedGame::~NetworkedGame()	{
	delete thisServer;
	delete thisClient;
}

void NetworkedGame::StartAsServer() {
	thisServer = new GameServer(NetworkBase::GetDefaultPort(), 4);

	thisServer->RegisterPacketHandler(Received_State, this);
	thisServer->RegisterPacketHandler(Player_Connected, this);
	thisServer->RegisterPacketHandler(Player_Disconnected, this);
	thisServer->RegisterPacketHandler(Client_Update, this);

	myID = 10000;
	ServerStartLevel();
}

void NetworkedGame::StartAsClient(char a, char b, char c, char d) {
	thisClient = new GameClient();
	if (!thisClient->Connect(a, b, c, d, NetworkBase::GetDefaultPort()))
		std::cout << "Could not connect!\n";

	thisClient->RegisterPacketHandler(Delta_State, this);
	thisClient->RegisterPacketHandler(Full_State, this);
	thisClient->RegisterPacketHandler(Player_Connected, this);
	thisClient->RegisterPacketHandler(Player_Disconnected, this);
	thisClient->RegisterPacketHandler(Message, this);

	StartLevel();
}

void NetworkedGame::UpdateGame(float dt) {
	Debug::Print("Player ID: " + std::to_string(myID), Vector2(0, 5));
	if (thisServer)
		gameDurationRemaining -= dt;
	timeToNextPacket -= dt;
	if (timeToNextPacket < 0) {
		if (thisServer) {
			UpdateAsServer(dt);
		}
		else if (thisClient) {
			UpdateAsClient(dt);
		}
		timeToNextPacket += 1.0f / 20.0f; //20hz server/client update
	}

	if (thisServer) {
		if (gameDurationRemaining < -5) {
			if (finalScore > highScore) {
				highScore = finalScore;
			}
			ServerStartLevel();
		}
		if (gameDurationRemaining < 0 && winner == 0) {
			int bestScore = goat->GetScore();
			int bestId = myID;
			for (auto i = serverPlayers.begin(); i != serverPlayers.end(); i++) {
				if ((*i).second->GetScore() > bestScore) {
					bestScore = (*i).second->GetScore();
					bestId = (*i).first;
				}
			}
			winner = bestId;
			finalScore = bestScore;
			MessagePacket winnerPacket;
			winnerPacket.messageID = 4;
			winnerPacket.messageValue = winner;
			winnerPacket.messageValueTwo = bestScore;
			winnerPacket.highScore = highScore;
			thisServer->SendGlobalPacket(winnerPacket);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
			ServerStartLevel();
			return;
		}
		for (auto i = serverPlayers.begin(); i != serverPlayers.end(); i++) {
			if ((*i).first == myID)
				continue;
			Goat* goat = (*i).second;
			PlayerControl* controls = playerControls.find((*i).first)->second;
			GoatControl(dt, goat, controls->camera, controls->buttonstates[0] == 1, controls->buttonstates[1] == 1, controls->buttonstates[2] == 1, controls->buttonstates[3] == 1);
		}
	}

	if (winner != 0) {
		Debug::Print("Winner: " + std::to_string(winner), Vector2(0, 20));
		Debug::Print("Winner Score: " + std::to_string(finalScore), Vector2(0, 25));
		if(finalScore > highScore)
			Debug::Print("NEW SERVER HIGHSCORE!", Vector2(0, 30));
	}

	if (!thisServer && Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		StartAsServer();
	}
	if (!thisClient && Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		StartAsClient(127,0,0,1);
	}

	TutorialGame::UpdateGame(dt);
}

void NetworkedGame::UpdateAsServer(float dt) {
	thisServer->UpdateServer();
	myState++;
	packetsToSnapshot--;
	if (packetsToSnapshot < 0) {
		UpdateMinimumState();
		BroadcastSnapshot(false);
		packetsToSnapshot = 5;
	}
	else {
		BroadcastSnapshot(true);
	}
	MessagePacket timer;
	timer.messageID = 3;
	timer.messageValue = (int)gameDurationRemaining;
	thisServer->SendGlobalPacket(timer);
}

void NetworkedGame::UpdateAsClient(float dt) {
	thisClient->UpdateClient();

	ClientPacket newPacket;
	newPacket.playerID = myID;
	newPacket.lastID = myState;

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		//fire button pressed!
		newPacket.buttonstates[0] = 1;
	}
	else
		newPacket.buttonstates[0] = 0;
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		//fire button pressed!
		newPacket.buttonstates[1] = 1;
	}
	else
		newPacket.buttonstates[1] = 0;
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		//fire button pressed!
		newPacket.buttonstates[2] = 1;
	}
	else
		newPacket.buttonstates[2] = 0;
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		//fire button pressed!
		newPacket.buttonstates[3] = 1;
	}
	else
		newPacket.buttonstates[3] = 0;
	newPacket.camera = (int)world->GetMainCamera()->GetYaw();
	thisClient->SendPacket(newPacket);
}

void NetworkedGame::BroadcastSnapshot(bool deltaFrame) {
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;

	world->GetObjectIterators(first, last);

	for (auto i = first; i != last; ++i) {
		NetworkObject* o = (*i)->GetNetworkObject();
		if (!o) {
			continue;
		}
		int minID = INT_MAX;

		for (auto i : stateIDs) {
			minID = min(minID, i.second);
		}

		if (minID == INT_MAX)
			minID = myState - 5;

		GamePacket* newPacket = nullptr;
		if (o->WritePacket(&newPacket, deltaFrame, minID)) {
			thisServer->SendGlobalPacket(*newPacket);
			delete newPacket;
		}
	}
}

void NetworkedGame::UpdateMinimumState() {
	//Periodically remove old data from the server
	int minID = INT_MAX;
	int maxID = 0; //we could use this to see if a player is lagging behind?

	for (auto i : stateIDs) {
		minID = min(minID, i.second);
		maxID = max(maxID, i.second);
	}
	if (minID = INT_MAX)
		minID = myState - 10;
	//every client has acknowledged reaching at least state minID
	//so we can get rid of any old states!
	std::vector<GameObject*>::const_iterator first;
	std::vector<GameObject*>::const_iterator last;
	world->GetObjectIterators(first, last);

	for (auto i = first; i != last; ++i) {
		NetworkObject* o = (*i)->GetNetworkObject();
		if (!o) {
			continue;
		}
		o->UpdateStateHistory(minID); //clear out old states so they arent taking up memory...
	}
}

Goat* NetworkedGame::SpawnPlayer(int playerID) {
	Goat* newPlayerCharacter = AddPlayerToWorld();
	newPlayerCharacter->SetGame(this);
	NetworkObject* netO;
	netO = new NetworkObject((GameObject*)newPlayerCharacter, playerID, myState);
	serverPlayers.emplace(playerID, newPlayerCharacter);
	networkObjects.push_back(netO);
	return newPlayerCharacter;
}

void NetworkedGame::ServerStartLevel() {
	myState = 100;
	// Reset the game world
	StartLevel();
	// For clients too!
	MessagePacket resetPacket;
	resetPacket.messageID = 1;
	thisServer->SendGlobalPacket(resetPacket);
	// Add all goats in
	goat = SpawnPlayer(10000);
	ConnectPacket outPacket(10000, false);
	thisServer->SendGlobalPacket(outPacket);
	for (auto i = playerControls.begin(); i != playerControls.end(); i++) {
		SpawnPlayer((*i).first);
		outPacket = ConnectPacket((*i).first, false);
		thisServer->SendGlobalPacket(outPacket);
	}
}

void NetworkedGame::StartLevel() {
	networkObjects.clear();
	serverPlayers.clear();
	InitWorld(false);
	std::vector<GameObject*>::const_iterator first, last;
	world->GetObjectIterators(first, last);
	int gooseCount = 0;
	int civCount = 0;
	int ballCount = 0;
	winner = 0;

	for (auto i = first; i != last; i++) {
		if ((*i)->GetName()._Equal("Civilian")) {
			//std::cout << "Homer has arrived\n";
			NetworkObject* netO;
			netO = new NetworkObject(&*(*i), civCount++, myState);
			networkObjects.push_back(netO);
		}
		else if ((*i)->GetName()._Equal("Random Ball")) {
			//std::cout << "They see me ballin\n";
				NetworkObject* netO;
				netO = new NetworkObject(&*(*i), 5000 + ballCount++, myState);
				networkObjects.push_back(netO);
		}
		else if ((*i)->GetName()._Equal("Goose")) {
			//std::cout << "HONK HONK BITCHES\n";
			NetworkObject* netO;
			netO = new NetworkObject(&*(*i), 1000 + gooseCount++, myState);
			networkObjects.push_back(netO);
		}
		else if ((*i)->GetName()._Equal("MagicBall")) {
			//std::cout << "SPOOKY\n";
			NetworkObject* netO;
			netO = new NetworkObject(&*(*i), 2000 + gooseCount++, myState);
			networkObjects.push_back(netO);
		}
	}
}

void NetworkedGame::ReceivePacket(int type, GamePacket* payload, int source) {
	if (type == Player_Connected) {
		ConnectPacket* realPacket = (ConnectPacket*)payload;
		int receivedID = realPacket->playerID;
		if (realPacket->you) {
			myID = receivedID;
			//std::cout << "Recieved my ID, I am" << myID << std::endl;
			return;
		}
		//std::cout << "Recieved message Player Connected, Spawning their goat, they are player ID" << receivedID << std::endl;
		Goat* newGoat = SpawnPlayer(receivedID);
		if (myID == receivedID) {
			goat = newGoat;
		}
		if (thisServer) {
			ConnectPacket outPacket(receivedID, false);
			thisServer->SendGlobalPacket(outPacket);
			stateIDs.emplace(receivedID, 0);
			playerControls.emplace(receivedID, new PlayerControl());
			for (auto i = serverPlayers.begin(); i != serverPlayers.end(); i++) {
				if ((*i).first != receivedID) {
					ConnectPacket goatPacket((*i).first, false);
					thisServer->SendPacketToClient(goatPacket, receivedID);
				}
			}
		}
	}
	else if (type == Player_Disconnected) {
		DisconnectPacket* realPacket = (DisconnectPacket*)payload;
		int receivedID = realPacket->playerID;
		//std::cout << "Recieved message Player Disconnected, removing their goat, they are player ID" << receivedID << std::endl;
		Goat* removingGoat = serverPlayers.find(receivedID)->second;
		world->RemoveGoat((GameObject*)removingGoat, true);
		serverPlayers.erase(receivedID);
		if (thisServer) {
			delete playerControls.find(receivedID)->second;
			playerControls.erase(receivedID);
			stateIDs.erase(receivedID);
			DisconnectPacket outPacket(receivedID);
			thisServer->SendGlobalPacket(outPacket);
		}
	}
	else if (type == Client_Update) {
		ClientPacket* realPacket = (ClientPacket*)payload;
		int receivedID = realPacket->playerID;
		//std::cout << "Recieved message Client Update, adjusting their controls, they are player ID" << receivedID << " and in State " << realPacket->lastID << std::endl;
		if (receivedID < 10000) {
			//std::cout << "The client doesnt seem to have their ID/goat yet, ignoring\n";
			return;
		}
		stateIDs.find(receivedID)->second = realPacket->lastID;
		PlayerControl* playersControls = playerControls.find(receivedID)->second;
		if (playersControls->buttonstates[0] != realPacket->buttonstates[0]) {
			playersControls->buttonstates[0] = realPacket->buttonstates[0];
			//std::cout << "Client " << receivedID << ": CHANGE IN UP KEY\n";
		}
		if (playersControls->buttonstates[1] != realPacket->buttonstates[1]) {
			playersControls->buttonstates[1] = realPacket->buttonstates[1];
			//std::cout << "Client " << receivedID << ": CHANGE IN DOWN KEY\n";
		}
		if (playersControls->buttonstates[2] != realPacket->buttonstates[2]) {
			playersControls->buttonstates[2] = realPacket->buttonstates[2];
			//std::cout << "Client " << receivedID << ": CHANGE IN LEFT KEY\n";
		}
		if (playersControls->buttonstates[3] != realPacket->buttonstates[3]) {
			playersControls->buttonstates[3] = realPacket->buttonstates[3];
			//std::cout << "Client " << receivedID << ": CHANGE IN RIGHT KEY\n";
		}
		playersControls->camera = realPacket->camera;
	}
	else if (type == Full_State) {
		FullPacket* realPacket = (FullPacket*)payload;
		//std::cout << "Recieved FullPacket for object " << realPacket->objectID << " at object state " << realPacket->fullState.stateID << std::endl;
		myState = max(myState, realPacket->fullState.stateID);
		for (auto i : networkObjects)
			if (i->GetNetworkID() == realPacket->objectID) {
				i->ReadPacket(*realPacket);
				break;
			}
	}
	else if (type == Delta_State) {
		DeltaPacket* realPacket = (DeltaPacket*)payload;
		//std::cout << "Recieved DeltaPacket for object " << realPacket->objectID << " at object state " << realPacket->fullID << std::endl;
		for (auto i : networkObjects)
			if (i->GetNetworkID() == realPacket->objectID) {
				i->ReadPacket(*realPacket);
				break;
			}
	}
	else if (type == Message) {
		MessagePacket* realPacket = (MessagePacket*)payload;
		switch (realPacket->messageID) {
		case(1):
			myState = 0;
			StartLevel();
			break;
		case(2):
			goat->AddHitCivilian(stateGameObjects[realPacket->messageValue]);
			stateGameObjects[realPacket->messageValue]->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));
			break;
		case(3):
			gameDurationRemaining = realPacket->messageValue;
			break;
		case(4):
			winner = realPacket->messageValue;
			finalScore = realPacket->messageValueTwo;
			highScore = realPacket->highScore;
			break;
		default: std::cout << "Recieved unknown message\n";
		}
	}
}

void NetworkedGame::OnPlayerCivilianCollision(Goat* player, StateGameObject* civilian) {
	if (thisServer) {
		player->AddHitCivilian(civilian);
		if (player == goat) // If it's the server that hit
			civilian->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));
		else {
			MessagePacket newPacket;
			newPacket.messageID = 2;
			for (int i = 0; i < stateGameObjects.size(); i++)
				if (stateGameObjects[i] == civilian) {
					newPacket.messageValue = i;
					for (auto j = serverPlayers.begin(); j != serverPlayers.end(); j++)
						if ((*j).second == player) {
							thisServer->SendPacketToClient(newPacket, (*j).first);
							return;
						}
				}
		}
	}
}