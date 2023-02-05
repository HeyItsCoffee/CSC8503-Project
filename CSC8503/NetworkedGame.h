#pragma once
#include "TutorialGame.h"
#include "NetworkBase.h"

namespace NCL {

	namespace CSC8503 {
		class GameServer;
		class GameClient;
		class NetworkPlayer;

		struct PlayerControl {
			char	buttonstates[4]; // UP, LEFT, RIGHT, DOWN
			int		camera; // Yaw

			PlayerControl() {
				buttonstates[0] = 0;
				buttonstates[1] = 0;
				buttonstates[2] = 0;
				buttonstates[3] = 0;
				camera = 0;
			}
		};

		class NetworkedGame : public TutorialGame, public PacketReceiver {
		public:
			NetworkedGame();
			NetworkedGame(int a, int b, int c, int d);
			~NetworkedGame();

			void StartAsServer();
			void StartAsClient(char a, char b, char c, char d);

			void UpdateGame(float dt) override;

			Goat* SpawnPlayer(int playerID);

			void ServerStartLevel();
			void StartLevel();

			void ReceivePacket(int type, GamePacket* payload, int source) override;

			void OnPlayerCivilianCollision(Goat* goat, StateGameObject* civilian);

		protected:
			void UpdateAsServer(float dt);
			void UpdateAsClient(float dt);

			void BroadcastSnapshot(bool deltaFrame);
			void UpdateMinimumState();

			GameServer* thisServer;
			GameClient* thisClient;
			float timeToNextPacket;
			int packetsToSnapshot;

			int myID;
			int winner;
			int myState;

			std::vector<NetworkObject*> networkObjects;

			std::map<int, Goat*> serverPlayers;
			std::map<int, PlayerControl*> playerControls;
			std::map<int, int> stateIDs;
			GameObject* localPlayer;
		};
	}
}

