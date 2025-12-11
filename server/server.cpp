/**
 * server.cpp -- serveur de demonstration
 * a faire : ajouter la gestion des erreurs
 * a faire : faire le traitement pour le fils dans un thread
**/
#include <iostream>
#include <thread>
#include <csignal>

#include <ctime>
#include <clocale>
#include "socket.h"

using namespace std;
using namespace stdsock;

string valRandom (){

    string tabChoix[3] = {"p","c","f"};
    string val = tabChoix[rand()%3];

    if(val == "p"){
        return "Pierre";
    }else if(val == "f" ) {
        return "Feuille";
    }else if(val == "c" ){
        return "Ciseaux";
    }else return "Erreur";

}

void worker(StreamSocket* client){
    int score =0;
    client->send("GAME -> Jouer \nSTOP -> Arreter la connexion \nEXIT -> Sort d'une partie \n");
    while(true){
        //Recuperation msg
        std::string msg;
        client->read(msg);

        if(msg == "GAME\r\n"){
            string valJoueur;
            string gagnant;
            bool game =true;
            while(game){
                string valMachine = valRandom();
                cout << valMachine<<endl;

                client->read(valJoueur);

                cout << valJoueur<<endl;

                if(valJoueur == "EXIT\r\n"){
                    cout <<"exit" << endl;
                    gagnant="Vous avez quitté \n";
                    game =false;
                    client->send(gagnant);
                    break;
                }

                if(valJoueur==(valMachine+"\r\n")){
                    gagnant="Egaliter \n";
                }
                else if(valJoueur == "Feuille\r\n" && valMachine == "Pierre"){
                    gagnant="Joueur gagne \n";
                    score++;

                }

                else if(valJoueur == "Pierre\r\n" && valMachine == "Ciseaux"){
                    gagnant="Joueur gagne \n";
                    score++;
                }

                else if(valJoueur == "Ciseaux\r\n" && valMachine == "Feuille"){
                    gagnant="Joueur gagne \n";
                    score++;
                }
                else {
                    gagnant="Machine gagne \n";
                }

                client->send(gagnant + " score : " + std::to_string(score) + "\nEXIT pour arreter\n");

            }
        }

        // on termine le Client proprement
        if(msg == "STOP\r\n"){
            delete client;
            std::cout << "Client disconnected" << std::endl;
            break;
        }
    }

}



int main()
{
    srand(time(nullptr));
    // Creation et initialisation du point de connexion
    // Port 3490, backlog 10 par defaut (nombre de connexion en attente autorisees)
    ConnectionPoint *server=new ConnectionPoint(3490);

    // Init : Bind + Listen
    // Bind : Demarrage du point de connexion : on ajoute l'adresse de transport dans la socket
    // Listen : Attente sur le point de connexion
    int err= server->init();
    if (err != 0) {
        std::cout << "Server init(): " << strerror(err) << std::endl;
        exit(err);
    }

    // On affiche les informations sur le point de connexion du serveur
    std::cout << server->getIP() << ":" << server->getPort() << std::endl;

    // Boucle principale de traitement des demandes client
    while (true) {
        StreamSocket* client = server->accept();

        // si erreur
        if (!client->valid()) {
            delete client;
            std::cout << strerror(err) << std::endl;
            continue;
        }

        // Une connexion a eu lieu : on recupere l'adresse du client
        std::cout << "Got a client!" << std::endl;
        std::cout << client->getIP() << ":" << client->getPort() << std::endl;

        thread clientThread (worker,client);
        clientThread.detach();
    }

    // Note: le server tourne en boucle, jamais appelé...
    delete server;
    return 0;
}


