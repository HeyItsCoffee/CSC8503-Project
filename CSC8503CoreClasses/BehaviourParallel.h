#pragma once
#include "BehaviourNodeWithChildren.h"

class BehaviourParallel : public BehaviourNodeWithChildren {
public:
	BehaviourParallel(const std::string& nodeName) : BehaviourNodeWithChildren(nodeName) {}
	~BehaviourParallel() {}
	BehaviourState Execute(float dt) override {
		//std::cout << "Executing parallel " << name << "\n";
		currentState = Success;
		for (auto& i : childNodes) {
			BehaviourState nodeState = i->Execute(dt);
			switch (nodeState) {
				case Failure: 
				case Success: continue;
				case Ongoing:
				{
					currentState = nodeState;
				}
			}
		}
		return currentState;
	}
};

// If any are ongoing, return ongoing
// If all are success, return success
// All actions MUST try again once failed (so are now ongoing again)