#pragma once

#include <unordered_set>

template <typename T>
class SimpleObserver {
public:
	// Add a notification to the message vector
	void RecieveMessage(T* pObject) {
		messages.emplace(pObject);
	}

	// Return all notifications received
	virtual std::unordered_set<T*>& getNotifications() {
		return messages;
	}

	// Remove all previous notifications after properly handling those
	virtual void ClearPreviousNotifications() {
		messages.clear();
	}
private:
	// All notifications being received
	std::unordered_set<T*> messages;
};