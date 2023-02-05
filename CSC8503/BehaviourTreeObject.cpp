//
//Goose::Goose(GameWorld* gameWorld) : BehaviourTreeObject("Goose", gameWorld) {
//	rootNode = new BehaviourParallel("Root Node");
//
//	BehaviourAction* findNearestGoat = new BehaviourAction("Find Closest Goat",
//		[&, gameWorld](float dt, BehaviourState state) -> BehaviourState {
//			std::cout << "Looking for any goat...";
//			vector<GameObject*> players = gameWorld->GetGoats();
//			float bestDistance = -1;
//			GameObject* bestGoat;
//			Vector3 goosePosition = GetTransform().GetPosition();
//			for (GameObject* goat : players) {
//				// Check goat is close
//				Vector3 goatPosition = goat->GetTransform().GetPosition();
//				Vector3 positionDifference = goatPosition - goosePosition;
//				float distance = positionDifference.Length();
//			}
//			return Ongoing;
//		});
//}