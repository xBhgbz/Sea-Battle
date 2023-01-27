#pragma once
// Ports for messages
const char kServerPort[] = "tcp://localhost:5555";
const char kClientPort[] = "tcp://*:5555";


// In message delimiter
const char kMessagePartsDelimiter = '#';
// Message delimiter
const char kMessageDelimiter = '$';


// Types of messages (sort by req/res)
// REQUEST -> RESPOND
// Login request
const char kLogin = 'L'; // [L#Login] req -> [L] res

// Create game request
const char kCreateGame = 'C'; // [C#UID#GameName] req -> [C] res

// Get list of available games request
const char kGetGameList = 'G'; // [G#UID] req -> [G#Game1#Game2...] res

// Send game field request
const char kFieldCheck = 'M'; // [M#UID#GameName#Map] req -> [M] res

// Player's move request
const char kDoAction = 'D'; // [D#UID#GameName#MyAction] req -> [D#Result] res

// Invite player request
const char kInvitePlayer = 'I'; // [I#UID#Login1#GameName] req -> [I] res
// Invited player gets [I#Login#GameName] res -> and then can [J#UID#GameName] req

// Join game request
const char kJoinGame = 'J'; // [J#UID#GameName] req -> [J] res

// Get saved messages request
const char kNothing = 'N'; // [N#UID]



// RESPONDS
// Respond to start game
const char kStartGame = 'S'; // [S#(Y/N)] res  (Y - you start, N - not you)

// Respond with enemy move
const char kEnemyAction = 'Y'; // [Y#RowColumnResult] res 

// Respond that game ends
const char kGameEnd = 'E'; // [E#Winner] res

// Respond that player joined game
const char kPlayerJoinYourGame = 'P'; // [P#Login] res

// Fail respond
const char kFailure = 'F';



// Some consants for game field and messaging
const int kSea = 0;
const int kShip = 1;
const int kDamagedShip = 2;
const int kDamagedSea = 3;
const int kUnknownTile = 4;
const int kDestroyed = 5;