#include <iostream>
#include "socket.h"

using namespace std;
using namespace stdsock;

int main(int argc, char* argv[])
{
    int port;
    string ip;

    // parametrage du port et de l'adresse IP du serveur (point de connexion)
    port = 3490;

    ip = "127.0.0.1";

    // creation de la socket
    StreamSocket* socket = new StreamSocket(ip,port);

    // tentative de connexion sur le serveur
    int errConnect = socket->connect();

    // gestion des erreurs
    if(errConnect !=0){
        std::cout << "probleme de connexion" << strerror(errConnect) << std::endl;
        exit(errConnect);
    }

    //affichage des informations du serveur
    std::cout << socket->getIP() << ":" << socket->getPort() << std::endl;

    // reception du message
    std::string msg;
    int errMessage = socket->read(msg);



    // gestion des erreurs
    if(errMessage==-1){
        std::cout << "erreur dans la lecture du message" << std::endl;
        exit(errMessage);
    }

    // affichage du message
    std::cout << msg << std::endl;

    // fermeture de la socket
    socket->close();

    return 0;
}
