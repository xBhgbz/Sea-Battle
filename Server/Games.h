#pragma once

typedef struct structGame {
    std::vector<std::vector<int>> field; // First player [0-9], second player [10-19]
    std::string player[2], name;
    int isStarted;
    structGame(std::string gameName, std::string playerName);
} Game;

__declspec(selectany) std::vector<Game> games;

// Check if Game name is occupied
bool uniqueGameName(std::string name);

// Gets number of game in games
int searchGameByName(std::string name);
