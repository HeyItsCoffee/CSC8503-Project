#pragma once
#include "GameObject.h"

namespace NCL {
    namespace CSC8503 {
        class StateMachine;
        class GameWorld;
        class StateGameObject : public GameObject {
        public:
            StateGameObject(std::string name, GameWorld* gameWorld);
            ~StateGameObject();

            virtual void Update(float dt);

        protected:

            StateMachine* stateMachine;
            GameWorld* gameWorld;
        };

        class MagicBall : public StateGameObject {
        public:
            MagicBall(std::string name, GameWorld* gameWorld);

            void SetStartPos(Vector3 startPos) {
                this->startPos = startPos;
            }

        protected:
            Vector3 startPos;
            void MoveLeft(float dt);
            void MoveRight(float dt);
            void MoveForward(float dt);
            void MoveBack(float dt);
        };

        class Civilian : public StateGameObject {
        public:
            Civilian(std::string name, GameWorld* gameWorld);

        protected:
            int fov;
            // Originally every degree was checked but this was very expensive to do... checking only every 10 degrees is basically just as good and much faster!
            int step;
            void MoveForward(float dt);
            void TurnLeft(float dt);
            void TurnRight(float dt);
        };
    }
}
