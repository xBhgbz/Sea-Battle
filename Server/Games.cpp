#include <vector>
#include <iostream>

#include "Games.h"

structGame::structGame(std::string gameName, std::string playerUID) {
    name = gameName;

    field = std::vector<std::vector<int>>(10, std::vector<int>(20, 0));
    player[0] = playerUID;
    isStarted = -1;

    std::cout << "Game created with name {" << gameName << "}." << std::endl;
}

// Check if Game name is occupied
bool uniqueGameName(std::string name) {
    for (Game g : games)
        if (g.name == name)
            return false;
    return true;
}

// Gets number of game in games
int searchGameByName(std::string name) {
    for (int i = 0; i < games.size(); ++i)
        if (games[i].name == name)
            return i;
    return -1;
}
