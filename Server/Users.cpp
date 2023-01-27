#include <iostream>
#include <vector>
#include <random>
#include <string>

#include "Users.h"
#include "ServerConnection.h"

structUser::structUser(std::string userLogin) {
    login = userLogin;

    std::mt19937 mt_rand(time(0));
    do 
        uniqueID = std::to_string(mt_rand());
    while (!uniqueIdentity(uniqueID));

    std::cout << "Create user {" << login << "} with UID [" << uniqueID << "]" << std::endl;
}

// Check if UID is occupied
bool uniqueIdentity(std::string uniqueID) {
    for (User user : users)
        if (user.uniqueID == uniqueID)
            return false;
    return true;
}

// Check if login is occupied
bool uniqueUserLogin(std::string name) {
    for (User user : users)
        if (user.login == name)
            return false;
    return true;
}

// Gets number of user in users by UID
int searchUserByUID(std::string uniqueID) {
    for (int i = 0; i < users.size(); ++i)
        if (users[i].uniqueID == uniqueID)
            return i;
    return -1;
}

// Gets number of user in users by Login
int searchUserByLogin(std::string login) {
    for (int i = 0; i < users.size(); ++i)
        if (users[i].login == login)
            return i;
    return -1;
}

// Adds specific message for user
void addMessageToUser(int userNumber, std::string message) {
    if (users[userNumber].message.empty()) {
        users[userNumber].message = message;
        return;
    }
    users[userNumber].message += std::string(1, kMessageDelimiter) + message;
}
