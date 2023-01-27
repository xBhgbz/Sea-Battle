#include <zmq.hpp>
#include <string>
#include <iostream>
#include <Windows.h>
#include <queue>

#include "ServerConnection.h"

std::string userGameName; // Name of game room
std::string login; // User login
std::string uniqueID; // Unique sequence for every user
std::queue<std::string> savedMessages; // Additional messages from server 
std::vector<std::vector<int>> myField, enemyField; // Represents game field

zmq::context_t context(1); // Context for ZMQ
zmq::socket_t messageSocket(context, zmq::socket_type::req);  // Socket for messages


// Split string with delimiter
std::vector<std::string> splitString(std::string request, std::string delimiter) {
    std::vector<std::string> messages;
    if (request.find(delimiter) != std::string::npos) {
        int position = request.find(delimiter);
        do {
            messages.push_back(request.substr(0, position));
            request = request.substr(position + 1, request.length() - position);
            position = request.find(delimiter);
        } while (position != std::string::npos);
        messages.push_back(request);
    }
    else 
        messages.push_back(request);
    return messages;
}

// ===========================================================================================
// 
//                      Client - Server messaging
// 
// ===========================================================================================

// Get message from saved messages
std::string getSavedMessage() {
    if (savedMessages.empty())
        return "";

    std::string message = savedMessages.front();
    savedMessages.pop();
    return message;
}

// Send request and split respond into some messages. Get direct respond for request.               
std::string getServerRespond(const std::string& request) {
    zmq::message_t message(request);
    messageSocket.send(message, zmq::send_flags::none);
    messageSocket.recv(message, zmq::recv_flags::none);

    std::string respond = message.to_string();
    std::vector<std::string> messages = splitString(respond, std::string(1, kMessageDelimiter));

    // Logging
    //if (respond != "N")
    //    std::cout << "Get respond [" << respond << "]" << std::endl;

    for (int i = 1; i < messages.size(); ++i)
        savedMessages.push(messages[i]);

    if (messages[0] == std::string(1, kNothing)) 
        return getSavedMessage();

    return messages[0];
}

// Get next message without request. If there are saved messages, then returns them first
std::string getNextMessage() {
    std::string respond = getSavedMessage();
    if (respond.empty()) {
        std::string request = std::string(1, kNothing) + std::string(1, kMessagePartsDelimiter) + uniqueID;
        respond = getServerRespond(request);
    }
    return respond;
}
 
// Login procedure
void doLogin() {
    std::cout << "Please enter your login: ";
    std::cin >> login;
    std::string request = std::string(1, kLogin) + std::string(1, kMessagePartsDelimiter) + login;

    std::string respond = getServerRespond(request);

    while (respond[0] != kLogin) {
        std::cout << "This login has been already taken. Please try another one: ";
        std::cin >> login;
        request = std::string(1, kLogin) + std::string(1, kMessagePartsDelimiter) + login;

        respond = getServerRespond(request);
    }

    uniqueID = respond.substr(2, respond.length() - 2);
    std::cout << "Login success!" << std::endl << std::endl;
}

// ===========================================================================================
//
//                        Procedure for working with game field
//
// ===========================================================================================

// Convert char field symbols to int analog
int mapSymbolToNumber(char symbol) {
    switch (symbol) {
    case '.': return 0;
    case '@': return 1;
    default: return -1;
    }
}

// Procedure to send game field to server
void createField() {
    std::cout << "Input your field. 10 rows, 10 columns '@' = ship, '.' = sea:" << std::endl;

    std::vector<std::string> field(10);
    for (int i = 0; i < 10; ++i) 
        std::cin >> field[i];

    std::string request = std::string(1, kFieldCheck) + std::string(1, kMessagePartsDelimiter) + uniqueID
        + std::string(1, kMessagePartsDelimiter) + userGameName;
    for (int i = 0; i < 10; ++i) 
        request += std::string(1, kMessagePartsDelimiter) + field[i];
    
    std::string respond = getServerRespond(request);

    while (respond[0] != kFieldCheck) {
        std::cout << "Wrong field. Try another one:" << std::endl;

        field = std::vector<std::string>(10);
        for (int i = 0; i < 10; ++i) 
            std::cin >> field[i];
        
        request = std::string(1, kFieldCheck) + std::string(1, kMessagePartsDelimiter) + uniqueID
            + std::string(1, kMessagePartsDelimiter) + userGameName;
        for (int i = 0; i < 10; ++i) 
            request += std::string(1, kMessagePartsDelimiter) + field[i];
        
        respond = getServerRespond(request);
    }

    myField = std::vector<std::vector<int>>(10, std::vector<int>(10));
    for (int row = 0; row < 10; ++row) 
        for (int column = 0; column < 10; ++column) 
            myField[row][column] = mapSymbolToNumber(field[row][column]);

    enemyField = std::vector<std::vector<int>>(10, std::vector<int>(10, 4));
    std::cout << "Waiting for other player to finish." << std::endl;
}

// Convert int analog to char field symbols
char numberToMapSymbol(int number) {
    switch (number) {
    case 0: return ' ';
    case 1: return '@';
    case 2: return '*';
    case 3: return '.';
    case 4: return '?';
    default: return '!';
    }
}

// Print fields in console
void printGameField() {
    std::cout << std::endl;
    std::cout << "0123456789   0123456789" << std::endl << std::endl;

    for (int row = 0; row < 10; ++row) {
        for (int column = 0; column < 10; ++column) 
            std::cout << numberToMapSymbol(myField[row][column]);

        std::cout << " " << row << " ";
        for (int column = 0; column < 10; ++column) 
            std::cout << numberToMapSymbol(enemyField[row][column]);

        std::cout << std::endl;
    }
    std::cout << std::endl;
}

// ===========================================================================================
// 
//                               Game procces
// 
// ===========================================================================================

// Print winner of the game
void printWinner(const std::string& message) {
    std::string winner = message.substr(2, message.length() - 2);
    if (winner == login)
        std::cout << "You win!" << std::endl;
    else
        std::cout << "Player " << winner << " won!" << std::endl;
    std::cout << "Exiting to menu..." << std::endl << std::endl;
}

// Check if coordinate are correct
bool correctCoordinate(int number) {
    return number >= 0 && number < 10;
}

// Mark all fields around destroyed ship
void destroyShip(std::vector<std::vector<int>>& field, int row, int column) {
    std::queue<std::pair<int, int>> queue, editedTiles;
    queue.push(std::pair<int, int>(row, column));
    field[row][column] = 2;

    int dColumn[8] = { -1,  0,  1, -1, 1, -1, 0, 1 };
    int dRow[8] = { -1, -1, -1,  0, 0,  1, 1, 1 };
    while (!queue.empty()) {
        int tileRow = queue.front().first, tileColumn = queue.front().second;
        queue.pop();

        if (field[tileRow][tileColumn] == 0 || field[tileRow][tileColumn] == 4) {
            field[tileRow][tileColumn] = kDamagedSea;
            continue;
        }

        if (field[tileRow][tileColumn] == 2) {
            editedTiles.push(std::pair<int, int>(tileRow, tileColumn));
            field[tileRow][tileColumn] = kUnknownTile;

            for (int i = 0; i < 8; ++i)
                if (correctCoordinate(tileColumn + dColumn[i]) && correctCoordinate(tileRow + dRow[i]))
                    queue.push(std::pair<int, int>(tileRow + dRow[i], tileColumn + dColumn[i]));
        }
    }

    while (!editedTiles.empty()) {
        int tileRow = editedTiles.front().first, tileColumn = editedTiles.front().second;
        editedTiles.pop();
        field[tileRow][tileColumn] = kDamagedShip;
    }
}

// Handle enemy move. Return true if enemy is still moving.
bool hadleEnemyMove(const std::string& message) {
    int row = message[2] - '0', column = message[3] - '0', result = message[4] - '0';
    if (result == kDestroyed)
        destroyShip(myField, row, column);
    else
        myField[row][column] = result;

    std::cout << "Enemy action: " << std::endl;
    printGameField();

    if (result == kDestroyed || result == kDamagedShip)
        return true;
    return false;
}

// Do your move
void makeMove() {
    int row, column, result;

    do {
        std::cout << "Enter coordinates of shot: ";
        std::cin >> row >> column;

        while (enemyField[row][column] != 4) {
            std::cout << "You alredy shot there! Enter another coordinates: ";
            std::cin >> row >> column;
        }

        std::string message = std::string(1, kDoAction) + std::string(1, kMessagePartsDelimiter)
            + uniqueID + std::string(1, kMessagePartsDelimiter) + userGameName + std::string(1, kMessagePartsDelimiter)
            + std::string(1, row + '0') + std::string(1, column + '0');

        message = getServerRespond(message);

        if (message[0] == kGameEnd) {
            savedMessages.push(message);
            return;
        }

        result = message[2] - '0';
        if (result == kDestroyed)
            destroyShip(enemyField, row, column);
        else 
            enemyField[row][column] = result;
        printGameField();


        message = getSavedMessage();
        while (message != "") {
            if (message[0] == kGameEnd) {
                savedMessages.push(message);
                return;
            }
            message = getSavedMessage();
        }
    } while (result == kDamagedShip || result == kDestroyed);
}


// Game procedure
void playGame() {
    createField();
    printGameField();

    std::string message;
    while (true) {
        message = getNextMessage();

        if (message[0] == kStartGame) {
            if (message[2] == 'Y')
                makeMove();
            break;
        }
    }

    while (true) {
        message = getNextMessage();

        if (message[0] == kGameEnd) {
            printWinner(message);
            break;
        }

        if (message[0] == kEnemyAction) {
            if (hadleEnemyMove(message))
                continue;
            makeMove();
        }
    }
}

// ===========================================================================================
// 
//                               Game lobby procedures
// 
// ===========================================================================================

// Print lobby menu
void printLobbyMenu() {
    std::cout << "You are in game lobby. List of commands: " << std::endl;
    std::cout << "1. Invite player by login;" << std::endl;
    std::cout << "2. Refresh terminal." << std::endl << std::endl;
}

// Procedure of sending invite to player
void invitePlayerToGame() {
    std::string login;
    std::cout << "Enter user login: ";
    std::cin >> login;

    std::string message = std::string(1, kInvitePlayer) + std::string(1, kMessagePartsDelimiter)
        + uniqueID + std::string(1, kMessagePartsDelimiter) + login + std::string(1, kMessagePartsDelimiter)
        + userGameName;

    message = getServerRespond(message);

    if (message[0] == kFailure) {
        std::cout << "There is no such user." << std::endl << std::endl;
        return;
    }

    std::cout << "Invite was sent." << std::endl << std::endl;
}

// Game lobby procedure
void gameLobby() {
    int command;
    std::string message;
    while (true) {
        message = getNextMessage();

        if (message[0] == kPlayerJoinYourGame) {
            std::cout << "Player " << message.substr(2, message.length() - 2) << " joined your game."
                << std::endl << std::endl;
            playGame();
            break;
        }

        printLobbyMenu();

        std::cout << "You are in game lobby. Enter number of command: ";
        std::cin >> command;

        if (command == 1) 
            invitePlayerToGame();
    }
}

// ===========================================================================================
// 
//                                 Main menu procedures
// 
// ===========================================================================================

// Creating new game lobby
void createNewGame() {
    std::cout << "Enter game's name : ";
    std::string gameName;
    std::cin >> gameName;

    std::string message = std::string(1, kCreateGame) + std::string(1, kMessagePartsDelimiter)
        + uniqueID + std::string(1, kMessagePartsDelimiter) + gameName;

    message = getServerRespond(message);

    if (message[0] == kFailure) {
        std::cout << "Failed to create game. This name is already taken." << std::endl << std::endl;
        return;
    }

    userGameName = gameName;
    std::cout << "The lobby created successfully." << std::endl << std::endl;

    gameLobby();
}

// Getting list of available games
void viewGameList() {
    std::string message = std::string(1, kGetGameList) + std::string(1, kMessagePartsDelimiter)  + uniqueID;
    message = getServerRespond(message);

    std::vector<std::string> gameList = splitString(message, std::string(1, kMessagePartsDelimiter));
    std::cout << "List of available games: " << std::endl;
    for (int i = 1; i < gameList.size(); ++i) 
        std::cout << gameList[i] << ";" << std::endl;

    std::cout << std::endl;
}

// Joining game with name. if name is empty then console input.
void joinGame(const std::string& name) {
    std::string message, gameName;
    if (name.empty()) {
        std::cout << "Enter name of game you want to join: ";
        std::cin >> gameName;
    }
    else 
        gameName = name;
    
    message = std::string(1, kJoinGame) + std::string(1, kMessagePartsDelimiter) + uniqueID
        + std::string(1, kMessagePartsDelimiter) + gameName;

    message = getServerRespond(message);

    if (message[0] == kFailure) {
        std::cout << "Unable to join game. Game lobby are full or game does not exist."
            << std::endl << std::endl;
        return;
    }

    userGameName = gameName;
    std::cout << "You are joining game " << gameName << "." << std::endl << std::endl;

    playGame();
}

// Handle invite from other player
void handleInvite(const std::string& message) {
    std::vector<std::string> splitedString = splitString(message, std::string(1, kMessagePartsDelimiter));

    std::cout << splitedString[1] << " invite you to game: " << splitedString[2] << std::endl;
    std::cout << "'A' - accept, anything else - decline: " << std::endl;
    std::string answer;
    std::cin >> answer;
    if (answer == "A") 
       joinGame(splitedString[2]);
}

// Print main menu 
void printBaseMenu() {
    std::cout << "List of commands: " << std::endl;
    std::cout << "1. Create new game;" << std::endl;
    std::cout << "2. View game list;" << std::endl;
    std::cout << "3. Join game;" << std::endl;
    std::cout << "4. Print menu;" << std::endl;
    std::cout << "5. Refresh terminal." << std::endl << std::endl;
}





int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "          Welcome to Sea Battle game       " << std::endl;
    std::cout << "===========================================" << std::endl;

    messageSocket.connect(kServerPort);
    doLogin();

    printBaseMenu();
    int command;
    while (true) {
        std::string message = getNextMessage();

        if (message[0] == kInvitePlayer) {
            handleInvite(message);
            continue;
        }

        std::cout << "You are in game menu. Enter number of command: ";
        std::cin >> command;

        switch (command) {
        case 1:
            createNewGame();
            break;
        case 2:
            viewGameList();
            break;
        case 3:
            joinGame("");
            break;
        case 4:
            printBaseMenu();
            break;
        }
    }
}
