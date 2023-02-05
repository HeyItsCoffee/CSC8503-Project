#include "GameObject.h"
#include "../CSC8503/StateGameObject.h"
#include <set>
#include "RenderObject.h"
#include "Vector4.h"
#include "../CSC8503/NetworkedGame.h"

namespace NCL::CSC8503 {

	class Goat : public GameObject	{
	public:
		Goat(std::string name) : GameObject(name) {}

		void OnCollisionBegin(GameObject* otherObject) override {
			if (dynamic_cast<StateGameObject*>(otherObject)) {
				if (otherObject->GetName()._Equal("Civilian")) {
					StateGameObject* civilian = dynamic_cast<StateGameObject*>(otherObject);
					if (hitCivilians.count(civilian) == 0) {
						if (game)
							game->OnPlayerCivilianCollision(this, civilian);
						else {
							AddHitCivilian(civilian);
							civilian->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));
						}
					}}
			}
		}

		int GetScore() const { return hitCivilians.size(); }

		void SetGame(NetworkedGame* game) {
			this->game = game;
		}

		void AddHitCivilian(StateGameObject* civilian) {
			hitCivilians.insert(civilian);
		}
	protected:
		std::set<StateGameObject*> hitCivilians;

		NetworkedGame* game;
	};
}

