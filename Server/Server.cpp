#include <zmq.hpp>
#include <string>
#include <iostream>
#include <Windows.h>
#include <random>
#include <vector>
#include <queue>

#include "ServerConnection.h"
#include "Games.h"
#include "Users.h"

HANDLE hUsersMutex; // Mutex for users
HANDLE hGamesMutex; // Mutex for games
const int kMaxThreads = 8; // Max workers thread count
const char kWorkersPort[] = "inproc://workers"; // Port for workers

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
    else {
        messages.push_back(request);
    }
    return messages;
}

// ===========================================================================================
// 
//                                    Request Handlers
// 
// ===========================================================================================

// Login request handler
std::string userLoginHandler(const std::vector<std::string>& message) {
    std::string login = message[1];
    std::string respond;
    WaitForSingleObject(hUsersMutex, INFINITE);

    if (!uniqueUserLogin(login)) {
        respond = std::string(1, kFailure);
        ReleaseMutex(hUsersMutex);
        return respond;
    }

    User newUser = User(login);
    users.push_back(newUser);
    ReleaseMutex(hUsersMutex);

    respond = std::string(1, kLogin) + std::string(1, kMessagePartsDelimiter) + newUser.uniqueID;
    return respond;
}

// Create game request handler
std::string createGameHandler(const std::vector<std::string>& message) {
    std::string gameName = message[2];
    WaitForSingleObject(hGamesMutex, INFINITE);

    if (!uniqueGameName(gameName)) {
        ReleaseMutex(hGamesMutex);
        return std::string(1, kFailure);
    }

    std::string uniqueID = message[1];
    Game newGame = Game(gameName, uniqueID);
    games.push_back(newGame);
    ReleaseMutex(hGamesMutex);

    WaitForSingleObject(hUsersMutex, INFINITE);
    int playerNumber = searchUserByUID(uniqueID);
    users[playerNumber].gameName = gameName;
    ReleaseMutex(hUsersMutex);

    return std::string(1, kCreateGame);
}

// Get game list request handler
std::string getGameListHandler(const std::vector<std::string>& message) {
    std::string respond = std::string(1, kGetGameList);
    WaitForSingleObject(hGamesMutex, INFINITE);

    for (int i = 0; i < games.size(); ++i) 
        if (games[i].player[1].empty())
            respond += std::string(1, kMessagePartsDelimiter) + games[i].name;

    ReleaseMutex(hGamesMutex);
    return respond;
}

// Join game request handler
std::string joinGameHandler(const std::vector<std::string>& message) {
    WaitForSingleObject(hGamesMutex, INFINITE);
    int gameNumber = searchGameByName(message[2]);

    if (gameNumber == -1) {
        ReleaseMutex(hGamesMutex);
        return std::string(1, kFailure);
    }

    if (!games[gameNumber].player[0].empty() && !games[gameNumber].player[1].empty()) {
        ReleaseMutex(hGamesMutex);
        return std::string(1, kFailure);
    }

    games[gameNumber].player[1] = message[1];
    WaitForSingleObject(hUsersMutex, INFINITE);
    int waitingPlayerNumber = searchUserByUID(games[gameNumber].player[0]);
    ReleaseMutex(hGamesMutex);

    int joinedUserNumber = searchUserByUID(message[1]);
    users[joinedUserNumber].gameName = message[2];

    std::string additionalMessage = std::string(1, kPlayerJoinYourGame) + std::string(1, kMessagePartsDelimiter)
        + users[joinedUserNumber].login;
    addMessageToUser(waitingPlayerNumber, additionalMessage);

    ReleaseMutex(hUsersMutex);

    return std::string(1, kJoinGame);
}

// Invite player request handler
std::string invitePlayerHandler(const std::vector<std::string>& message) {
    WaitForSingleObject(hUsersMutex, INFINITE);
    int joinUserNumber = searchUserByLogin(message[2]);

    if (joinUserNumber == -1) {
        ReleaseMutex(hUsersMutex);
        return std::string(1, kFailure);
    }

    int inviterUserNumber = searchUserByUID(message[1]);
    std::string additionalMessage = std::string(1, kInvitePlayer) + std::string(1, kMessagePartsDelimiter)
        + users[inviterUserNumber].login + std::string(1, kMessagePartsDelimiter) + message[3];
    addMessageToUser(joinUserNumber, additionalMessage);
    ReleaseMutex(hUsersMutex);

    return std::string(1, kJoinGame);
}

// Convert char field symbols to int analog
int mapSymbolToNumber(char symbol) {
    switch (symbol) {
    case '.': return 0;
    case '@': return 1;
    default: return -1;
    }
}

// Game field request handler
std::string fieldCheckHandler(const std::vector<std::string>& message) {
    if (message.size() != 13) 
        return std::string(1, kFailure);

    std::vector<std::vector<int>> map(10, std::vector<int>(10));
    for (int row = 0; row < 10; ++row) {
        if (message[3 + row].size() != 10) {
            return std::string(1, kFailure);
        }

        for (int column = 0; column < 10; ++column) {
            map[row][column] = mapSymbolToNumber(message[3 + row][column]);
            if (map[row][column] == -1) 
                return std::string(1, kFailure);
        }
    }

    // Field is correct
    WaitForSingleObject(hGamesMutex, INFINITE);
    int gameNumber = searchGameByName(message[2]);

    int columnOffset; 
    if (message[1] == games[gameNumber].player[0]) 
        columnOffset = 0; 
    else 
        columnOffset = 10;

    for (int row = 0; row < 10; ++row) 
        for (int column = 0; column < 10; ++column) 
            games[gameNumber].field[row][columnOffset + column] = map[row][column];

    if (games[gameNumber].isStarted == -1) 
        games[gameNumber].isStarted = 0;
    else {
        WaitForSingleObject(hUsersMutex, INFINITE);
        int firstPlayerNumber = searchUserByUID(games[gameNumber].player[0]);
        int secondPlayerNumber = searchUserByUID(games[gameNumber].player[1]);

        std::string additionalMessage = std::string(1, kStartGame) + std::string(1, kMessagePartsDelimiter) + "Y";
        addMessageToUser(firstPlayerNumber, additionalMessage);

        additionalMessage = std::string(1, kStartGame) + std::string(1, kMessagePartsDelimiter) + "N";
        addMessageToUser(secondPlayerNumber, additionalMessage);

        ReleaseMutex(hUsersMutex);
    }
    ReleaseMutex(hGamesMutex);
    return std::string(1, kFieldCheck);
}

// Check if player have any alive ship
bool hasAliveShips(int gameNumber, int player) {
    for (int row = 0; row < 10; ++row) {
        for (int column = 0; column < 10; ++column) {
            if (games[gameNumber].field[row][column + 10 * player] == kShip)
                return true;
        }
    }
    return false;
}

// Check if coordinate is correct
bool correctCoordinate(int number) {
    return number >= 0 && number < 10;
}

// Check if ship is alive
bool isShipAlive(int gameNumber, int row, int column, int columnOffset) {
    std::queue<std::pair<int, int>> queue, editedTiles;
    queue.push(std::pair<int, int>(row, column));
    games[gameNumber].field[row][column] = 2;

    int dColumn[8] = { -1,  0,  1, -1, 1, -1, 0, 1 };
    int dRow[8] = { -1, -1, -1,  0, 0,  1, 1, 1 };
    while (!queue.empty()) {
        int tileRow = queue.front().first, tileColumn = queue.front().second;
        queue.pop();

        if (games[gameNumber].field[tileRow][tileColumn] == kDamagedShip) {
            games[gameNumber].field[tileRow][tileColumn] = kUnknownTile;
            editedTiles.push(std::pair<int, int>(tileRow, tileColumn));

            for (int i = 0; i < 8; ++i)
                if (correctCoordinate(tileColumn + dColumn[i] - columnOffset) && correctCoordinate(tileRow + dRow[i]))
                    queue.push(std::pair<int, int>(tileRow + dRow[i], tileColumn + dColumn[i]));
        }
        
        if (games[gameNumber].field[tileRow][tileColumn] == kShip) {
            while (!editedTiles.empty()) {
                tileRow = editedTiles.front().first, tileColumn = editedTiles.front().second;
                editedTiles.pop();
                games[gameNumber].field[tileRow][tileColumn] = kDamagedShip;
            }
            return true;
        }
    }

    while (!editedTiles.empty()) {
        int tileRow = editedTiles.front().first, tileColumn = editedTiles.front().second;
        editedTiles.pop();
        games[gameNumber].field[tileRow][tileColumn] = kDamagedShip;
    }
    return false;
}

// Player's move handler
std::string doActionHandler(const std::vector<std::string>& message) {
    WaitForSingleObject(hGamesMutex, INFINITE);
    int gameNumber = searchGameByName(message[2]);

    int currentPlayerNumber, columnOffset;
    if (message[1] == games[gameNumber].player[0]) {
        currentPlayerNumber = 0;
        columnOffset = 10;
    }
    else {
        currentPlayerNumber = 1;
        columnOffset = 0;
    }

    int row = message[3][0] - '0', column = message[3][1] - '0' + columnOffset, result;
    switch (games[gameNumber].field[row][column]) {
    case kSea:
        games[gameNumber].field[row][column] = kDamagedSea;
        result = kDamagedSea;
        break;
    case kShip:
        games[gameNumber].field[row][column] = kDamagedShip;
        result = kDamagedShip;
        if (!isShipAlive(gameNumber, row, column, columnOffset))
            result = kDestroyed;
        break;
    default:
        result = games[gameNumber].field[row][column];
        break;
    }
    
    WaitForSingleObject(hUsersMutex, INFINITE);
    int oppositePlayerNumber = searchUserByUID(games[gameNumber].player[1 - currentPlayerNumber]);
    std::string additionalMessage = std::string(1, kEnemyAction) + std::string(1, kMessagePartsDelimiter)
        + std::string(1, row + '0') + std::string(1, column + '0' - columnOffset) + std::string(1, result + '0');
    addMessageToUser(oppositePlayerNumber, additionalMessage);

    if (!hasAliveShips(gameNumber, 1 - currentPlayerNumber)) {
        int activePlayerNumber = searchUserByUID(games[gameNumber].player[currentPlayerNumber]);
        int loserPlayerNumber = oppositePlayerNumber;
        std::string message = std::string(1, kGameEnd) + std::string(1, kMessagePartsDelimiter)
            + users[activePlayerNumber].login;
        addMessageToUser(loserPlayerNumber, message);
        addMessageToUser(activePlayerNumber, message);

        games.erase(games.begin() + gameNumber);
    }

    ReleaseMutex(hGamesMutex);
    ReleaseMutex(hUsersMutex);

    return std::string(1, kDoAction) + std::string(1, kMessagePartsDelimiter) 
        + std::string(1, result + '0');
}

// ===========================================================================================
//
//                                   Worker thread
//
// ===========================================================================================

DWORD WINAPI workerThread(LPVOID arg) {
    zmq::context_t* context = (zmq::context_t*)arg;

    zmq::socket_t socket(*context, ZMQ_REP);
    socket.connect(kWorkersPort);

    while (true) {
        // Get request from client
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);
        std::string message = request.to_string();

        // Console log
        if (message[0] != kNothing)
            std::cout << "Received message [" << message << "]" << std::endl;

        // Handle message
        std::vector<std::string> messageParts = splitString(message, std::string(1, kMessagePartsDelimiter));
        switch (message[0]) {
        case kLogin:
            message = userLoginHandler(messageParts);
            break;
        case kCreateGame:
            message = createGameHandler(messageParts);
            break;
        case kGetGameList:
            message = getGameListHandler(messageParts);
            break;
        case kJoinGame:
            message = joinGameHandler(messageParts);
            break;
        case kInvitePlayer:
            message = invitePlayerHandler(messageParts);
            break;
        case kFieldCheck:
            message = fieldCheckHandler(messageParts);
            break;
        case kDoAction:
            message = doActionHandler(messageParts);
            break;
        default:
            message = std::string(1, kNothing);
            break;
        }

        // Attach saved messages
        if (messageParts[0][0] != kLogin) {
            int userNumber = searchUserByUID(messageParts[1]);
            if (!users[userNumber].message.empty()) {
                message += std::string(1, kMessageDelimiter) + users[userNumber].message;
                users[userNumber].message.erase();
            }
        }
        
        // Logging
        if (message != "N")
            std::cout << "Send respond [" << message << "]" << std::endl;

        // Send respond
        zmq::message_t reply(message);
        socket.send(reply, zmq::send_flags::none);
    }

    return 0;
}




int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "                  SERVER LOG               " << std::endl;
    std::cout << "===========================================" << std::endl;

    //  Prepare our context and sockets
    zmq::context_t context(1);
    zmq::socket_t clients(context, ZMQ_ROUTER);
    clients.bind(kClientPort);
    zmq::socket_t workers(context, ZMQ_DEALER);
    workers.bind(kWorkersPort);

    // Threads arrays
    HANDLE threads[kMaxThreads];
    DWORD dwThreadIdArray[kMaxThreads];

    // Mutexes
    hUsersMutex = CreateMutex(NULL, FALSE, NULL);
    hGamesMutex = CreateMutex(NULL, FALSE, NULL);

    //  Launch pool of worker threads
    for (int i = 0; i < kMaxThreads; ++i) {
        threads[i] = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size  
            workerThread,       // thread function name
            &context,          // argument to thread function 
            0,                      // use default creation flags 
            &dwThreadIdArray[i]
        );
    }

    // Creating Proxy between router and dealer
    zmq::socket_ref client(clients), worker(workers);
    zmq::proxy(client, worker);

    return 0;
}
