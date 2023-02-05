#include "NavigationGrid.h"
#include "Assets.h"

#include <fstream>
#include <CollisionDetection.h>

using namespace NCL;
using namespace CSC8503;

const int LEFT_NODE		= 0;
const int RIGHT_NODE	= 1;
const int TOP_NODE		= 2;
const int BOTTOM_NODE	= 3;

const char WALL_NODE	= 'x';
const char FLOOR_NODE	= '.';

NavigationGrid::NavigationGrid()	{
	nodeSize	= 0;
	gridWidth	= 0;
	gridHeight	= 0;
}

NavigationGrid::NavigationGrid(const std::string&filename, Vector3 zeroPos) : NavigationGrid() {


	this->zeroPos = zeroPos;
	std::ifstream infile(Assets::DATADIR + filename);

	infile >> nodeSize;
	infile >> gridWidth;
	infile >> gridHeight;

	for (int z = 0; z < gridHeight; ++z) {
		for (int x = 0; x < gridWidth; ++x) {
			int nodePos = (gridWidth * z) + x;
			allNodes.push_back(GridNode());
			char type = 0;
			infile >> type;
			allNodes[nodePos].type = type;
			allNodes[nodePos].position = zeroPos + Vector3((float)(x * nodeSize), 0, (float)(z * nodeSize));
		}
	}
	
	UpdateConnections();
}

void NavigationGrid::UpdateConnections() {
	//now to build the connectivity between the nodes
	for (int z = 0; z < gridHeight; ++z) {
		for (int x = 0; x < gridWidth; ++x) {
			GridNode& n = allNodes[(gridWidth * z) + x];

			if (z > 0) { //get the above node
				n.connected[0] = &allNodes[(gridWidth * (z - 1)) + x];
			}
			if (z < gridHeight - 1) { //get the below node
				n.connected[1] = &allNodes[(gridWidth * (z + 1)) + x];
			}
			if (x > 0) { //get left node
				n.connected[2] = &allNodes[(gridWidth * (z)) + (x - 1)];
			}
			if (x < gridWidth - 1) { //get right node
				n.connected[3] = &allNodes[(gridWidth * (z)) + (x + 1)];
			}
			for (int i = 0; i < 4; ++i) {
				if (n.connected[i]) {
					if (n.connected[i]->type == '.') {
						n.costs[i] = 1;
					}
					if (n.connected[i]->type == 'x') {
						n.connected[i] = nullptr; //actually a wall, disconnect!
						n.costs[i] = 0;
					}
				}
			}
		}
	}
}

void NavigationGrid::RemoveNode(Vector3 position) {
	position = position - zeroPos;
	int gridX = ((int)position.x / nodeSize);
	int gridZ = ((int)position.z / nodeSize);

	if (gridX < 0 || gridX > gridWidth - 1 ||
		gridZ < 0 || gridZ > gridHeight - 1) {
		return; //outside of map region!
	}

	GridNode& node = allNodes[(gridZ * gridWidth) + gridX];

	node.type = '.';
	UpdateConnections();
}

bool NavigationGrid::FullyWithinOpenNodes(Vector3 position, Vector3 halfSize) {
	Vector3 gridPos = position - zeroPos + Vector3(nodeSize / 2, 0, nodeSize / 2);
	Vector3 topLeft = gridPos - Vector3(halfSize.x, 0, halfSize.z);
	Vector3 botRight = gridPos + Vector3(halfSize.x, 0, halfSize.z);
	int gridTopX = ((int)topLeft.x / nodeSize);
	int gridTopZ = ((int)topLeft.z / nodeSize);
	int gridBotX = ((int)botRight.x / nodeSize);
	int gridBotZ = ((int)botRight.z / nodeSize);

	if (gridTopX < 0 || gridTopX > gridWidth - 1 ||
		gridTopZ < 0 || gridTopZ > gridHeight - 1) {
		return false; //outside of map region!
	}

	if (gridBotX < 0 || gridBotX > gridWidth - 1 ||
		gridBotZ < 0 || gridBotZ > gridHeight - 1) {
		return false; //outside of map region!
	}

	for (int z = gridTopZ; z <= gridBotZ; z++) {
		for (int x = gridTopX; x <= gridBotX; x++) {
			if (allNodes[(z * gridWidth) + x].type == 'x')
				return false;
		}
	}
	return true;
}

void NavigationGrid::Print() {
	for (int z = 0; z < gridHeight; ++z) {
		for (int x = 0; x < gridWidth; ++x) {
			GridNode& n = allNodes[(gridWidth * z) + x];
			char aChar = '0' + n.type;
			std::cout << aChar;
		}
		std::cout << std::endl;
	}
}

bool NavigationGrid::FindPath(const Vector3& from, const Vector3& to, NavigationPath& outPath) {
	//need to work out which node 'from' sits in, and 'to' sits in
	Vector3 gridFrom = from - zeroPos + Vector3(nodeSize/2, 0, nodeSize/2);
	Vector3 gridTo = to - zeroPos + Vector3(nodeSize / 2, 0, nodeSize / 2);
	int fromX = ((int)gridFrom.x / nodeSize);
	int fromZ = ((int)gridFrom.z / nodeSize);

	int toX = ((int)gridTo.x / nodeSize);
	int toZ = ((int)gridTo.z / nodeSize);

	if (fromX < 0 || fromX > gridWidth - 1 ||
		fromZ < 0 || fromZ > gridHeight - 1) {
		return false; //outside of map region!
	}

	if (toX < 0 || toX > gridWidth - 1 ||
		toZ < 0 || toZ > gridHeight - 1) {
		return false; //outside of map region!
	}

	GridNode* startNode = &allNodes[(fromZ * gridWidth) + fromX];
	GridNode* endNode	= &allNodes[(toZ * gridWidth) + toX];

	std::vector<GridNode*>  openList;
	std::vector<GridNode*>  closedList;

	openList.push_back(startNode);

	startNode->f = 0;
	startNode->g = 0;
	startNode->parent = nullptr;

	GridNode* currentBestNode = nullptr;

	while (!openList.empty()) {
		currentBestNode = RemoveBestNode(openList);

		if (currentBestNode == endNode) {			//we've found the path!
			GridNode* node = endNode;
			while (node != nullptr) {
				outPath.PushWaypoint(node->position);
				node = node->parent;
			}
			return true;
		}
		else {
			for (int i = 0; i < 4; ++i) {
				GridNode* neighbour = currentBestNode->connected[i];
				if (!neighbour) { //might not be connected...
					continue;
				}	
				bool inClosed	= NodeInList(neighbour, closedList);
				if (inClosed) {
					continue; //already discarded this neighbour...
				}

				float h = Heuristic(neighbour, endNode);				
				float g = currentBestNode->g + currentBestNode->costs[i];
				float f = h + g;

				bool inOpen		= NodeInList(neighbour, openList);

				if (!inOpen) { //first time we've seen this neighbour
					openList.emplace_back(neighbour);
				}
				if (!inOpen || f < neighbour->f) {//might be a better route to this neighbour
					neighbour->parent = currentBestNode;
					neighbour->f = f;
					neighbour->g = g;
				}
			}
			closedList.emplace_back(currentBestNode);
		}
	}
	return false; //open list emptied out with no path!
}

bool NavigationGrid::NodeInList(GridNode* n, std::vector<GridNode*>& list) const {
	std::vector<GridNode*>::iterator i = std::find(list.begin(), list.end(), n);
	return i == list.end() ? false : true;
}

GridNode*  NavigationGrid::RemoveBestNode(std::vector<GridNode*>& list) const {
	std::vector<GridNode*>::iterator bestI = list.begin();

	GridNode* bestNode = *list.begin();

	for (auto i = list.begin(); i != list.end(); ++i) {
		if ((*i)->f < bestNode->f) {
			bestNode	= (*i);
			bestI		= i;
		}
	}
	list.erase(bestI);

	return bestNode;
}

float NavigationGrid::Heuristic(GridNode* hNode, GridNode* endNode) const {
	return (hNode->position - endNode->position).Length();
}