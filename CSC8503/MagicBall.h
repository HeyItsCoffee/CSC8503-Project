#pragma once
#include "StateGameObject.h"

namespace NCL {
    namespace CSC8503 {
        class MagicBall : public StateGameObject {
        public:
            MagicBall(std::string name);

        protected:
            void MoveLeft(float dt);
            void MoveRight(float dt);
            void MoveForward(float dt);
            void MoveBack(float dt);
        };
    }
}
