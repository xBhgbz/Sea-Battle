#pragma once

typedef struct structUser {
    std::string uniqueID, login, gameName, message;
    structUser(std::string userLogin);
} User;

__declspec(selectany) std::vector<User> users;

// Check if UID is occupied
bool uniqueIdentity(std::string uniqueID);

// Check if login is occupied
bool uniqueUserLogin(std::string name);

// Gets number of user in users by UID
int searchUserByUID(std::string uniqueID);

// Gets number of user in users by Login
int searchUserByLogin(std::string login);

// Adds specific message for user
void addMessageToUser(int userNumber, std::string message);
