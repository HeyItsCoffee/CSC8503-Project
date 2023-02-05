#pragma once
#include "GameObject.h"
#include "BehaviourNodeWithChildren.h"
#include "BehaviourParallel.h"
#include "BehaviourAction.h"
#include "CollisionDetection.h"
#include "GameWorld.h"
#include "PhysicsObject.h"
#include <Maths.h>

namespace NCL {
    namespace CSC8503 {
        class StateMachine;
        class GameWorld;
        class BehaviourTreeObject : public GameObject {
        public:
            BehaviourTreeObject(std::string name, GameWorld* gameWorld) : GameObject(name) {
                this->gameWorld = gameWorld;
            }
            ~BehaviourTreeObject() {
                delete rootNode;
            }

            virtual void Execute(float dt) {
                rootNode->Execute(dt);
            }

        protected:
            BehaviourNodeWithChildren* rootNode;
            GameWorld* gameWorld;
        };

        // Implementing in .cpp file was causing strange issues, perhaps caused by cmake? so it's just done here instead
        class Goose : public BehaviourTreeObject {
        public:
            Goose(GameWorld* gameWorld) : BehaviourTreeObject("Goose", gameWorld) {
                rootNode = new BehaviourParallel("Root Node");

                BehaviourAction* findNearestGoat = new BehaviourAction("Find Closest Goat",
                    [&, gameWorld](float dt, BehaviourState state) -> BehaviourState {
                        vector<GameObject*> players = gameWorld->GetGoatsInMaze();
                        float bestDistance = -1;
                        GameObject* bestGoat = nullptr;
                        Vector3 goosePosition = GetTransform().GetPosition();
                        for (GameObject* goat : players) {
                            // Check if goat is in sight
                            Vector3 goatPosition = goat->GetTransform().GetPosition();
                            Vector3 positionDifference = goatPosition - goosePosition;
                            Ray ray = Ray(goosePosition, positionDifference.Normalised());
                            RayCollision gooseCollision;
                            if (gameWorld->Raycast(ray, gooseCollision, true)) {
                                if (gooseCollision.node == goat) {
                                    if (gooseCollision.rayDistance < bestDistance || bestDistance == -1) {
                                        bestDistance = gooseCollision.rayDistance;
                                        bestGoat = goat;
                                    }
                                    continue;
                                }
                            }
                            // Otherwise use pathfinding to see how far away the goat is
                            vector<Vector3> nodes = gameWorld->CalculatePathInMaze(goosePosition, goatPosition);
                            if (!nodes.empty()) {
                                float distance = (float)gameWorld->GetMaze()->GetNodeSize() * nodes.size();
                                if (distance < bestDistance || bestDistance == -1) {
                                    bestDistance = distance;
                                    bestGoat = goat;
                                }
                            }
                        }
                        if (bestGoat == nullptr) {
                            goatToChase = nullptr;
                            return Failure;
                        }
                        else {
                            goatToChase = bestGoat;
                        }
                        return Ongoing;
                    });

                BehaviourAction* moveToGoat = new BehaviourAction("Move to Goat",
                    [&, gameWorld](float dt, BehaviourState state) -> BehaviourState {
                        if (goatToChase == nullptr)
                            return Failure;
                        // First try and move directly to the goat
                        Vector3 goosePosition = GetTransform().GetPosition();
                        Vector3 goatPosition = goatToChase->GetTransform().GetPosition();
                        Vector3 positionDifference = goatPosition - goosePosition;
                        Ray ray = Ray(goosePosition, positionDifference.Normalised());
                        RayCollision gooseCollision;

                        if (gameWorld->Raycast(ray, gooseCollision, true))
                            if (gooseCollision.node == goatToChase) {
                                Debug::DrawLine(goosePosition, gooseCollision.collidedAt, Vector4(0, 1, 1, 1));
                                MoveTowards(dt * 2.5f, gooseCollision.collidedAt);
                                return Ongoing;
                            }
                        vector<Vector3> nodes = gameWorld->CalculatePathInMaze(goosePosition, goatPosition);
                        if (!nodes.empty()) {
                            Debug::DrawLine(goosePosition, nodes[0], Vector4(1, 1, 1, 1));
                            for (int i = 1; i < nodes.size(); i++) {
                                Vector3 a = nodes[i - 1];
                                Vector3 b = nodes[i];

                                Debug::DrawLine(a, b, Vector4(0, 1, 1, 1));
                            }
                            // Try to smooth the path taken with further nodes
                            if (nodes.size() > 1) {
                                Debug::DrawLine(nodes[nodes.size() - 1], goatPosition, Vector4(1, 1, 1, 1));
                                if (nodes.size() > 2)
                                    MoveTowards(dt, (nodes[1] + nodes[2]) / 2);
                                else
                                    MoveTowards(dt, nodes[1]);
                            }
                            else {
                                Debug::DrawLine(nodes[0], goatPosition, Vector4(1, 1, 1, 1));
                                MoveTowards(dt, nodes[0]);
                            }
                        }
                        else {
                            std::cout << " goat not in maze, how did we get here?\n";
                        }


                        return Ongoing;
                    });

                rootNode->AddChild(findNearestGoat);
                rootNode->AddChild(moveToGoat);
            }

        protected:
            GameObject* goatToChase;

            void MoveTowards(float dt, Vector3 target) {
                Vector3 gooseRotation = GetTransform().GetOrientation().ToEuler();
                Vector3 deltaPosition = GetTransform().GetPosition() - target;
                float yaw = gooseRotation.y;
                float directionYaw = atan2(deltaPosition.x, deltaPosition.z) * 180 / PI;
                if (yaw < 0)
                    yaw += 360;
                float yawDiff = directionYaw - yaw;
                if (yawDiff > 180)
                    yawDiff -= 360;
                else if (yawDiff < -180)
                    yawDiff += 360;

                Debug::DrawLine(GetTransform().GetPosition(), target);

                GetPhysicsObject()->AddTorque(Vector3(0, sqrt(abs(yawDiff)) * (yawDiff < 0 ? -1 : 1) * 10 * dt, 0));

                Vector3 direction = deltaPosition.Normalised();
                GetPhysicsObject()->AddForce(Vector3(-direction.x, 0, -direction.z) * dt * 200);
            }
        };
    }
}