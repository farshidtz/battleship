## Battleship

### How it works?
This application consists of two modules, the client side and the server side. One player should start the server side application and host the game at a desired port. After that, both players should run the client side application and will connect to the server providing its hostname and port. Each user should arrange their field before the game starts. Users can chat with each other during the game.

### Techniques used in this application are:
* Qt Socket Programming
* Qt Signals and Slots for handling incoming messages and responding to interactions in the interface
* Distinguish between the incoming socket messages (connection / shooting / chatting ) using RegEx

[Screenshot and more info](http://farshid.ws/projects/117)
