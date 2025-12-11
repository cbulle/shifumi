/**
 * server.cpp -- Serveur Shifumi (Pierre-Feuille-Ciseaux)
 * Gère 2 clients simultanément et détermine le gagnant
 */
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <ctime>
#include <cstdlib>
#include "socket.h"

using namespace std;
using namespace stdsock;

struct GameState {
    StreamSocket* client1;
    StreamSocket* client2;
    string choice1;
    string choice2;
    bool choice1Ready;
    bool choice2Ready;
    mutex mtx;
    condition_variable cv;
};

GameState gameState = {nullptr, nullptr, "", "", false, false};

string determineWinner(const string& c1, const string& c2) {
    string choice1 = c1.substr(0, c1.find("\r\n"));
    string choice2 = c2.substr(0, c2.find("\r\n"));
    
    if (choice1 == choice2) {
        return "DRAW";
    }
    
    if ((choice1 == "Pierre" && choice2 == "Ciseaux") ||
        (choice1 == "Feuille" && choice2 == "Pierre") ||
        (choice1 == "Ciseaux" && choice2 == "Feuille")) {
        return "PLAYER1";
    }
    
    return "PLAYER2";
}

void handleClient(StreamSocket* client, int clientNum) {
    try {
        string msg;
        int score = 0;
        
        client->send("Bienvenue au Shifumi!\nEn attente du deuxieme joueur...\n");
        
        // Attendre que les deux clients soient connectés
        {
            unique_lock<mutex> lock(gameState.mtx);
            if (clientNum == 1) {
                gameState.client1 = client;
            } else {
                gameState.client2 = client;
            }
            
            gameState.cv.wait(lock, []{ return gameState.client1 != nullptr && gameState.client2 != nullptr; });
            
            client->send("Joueur 2 connecte! Que le jeu commence!\n");
            client->send("Choix disponibles: Pierre, Feuille, Ciseaux\n");
        }
        
        while (true) {
            // Réinitialiser les choix
            {
                unique_lock<mutex> lock(gameState.mtx);
                gameState.choice1Ready = false;
                gameState.choice2Ready = false;
                gameState.choice1 = "";
                gameState.choice2 = "";
            }
            
            client->send("Votre choix (Pierre/Feuille/Ciseaux): ");
            client->read(msg);
            
            // Nettoyer le message
            string choice = msg.substr(0, msg.find("\r\n"));
            if (choice.empty() || choice.find("\r") != string::npos) {
                choice = msg;
            }
            
            // Valider le choix
            if (choice != "Pierre" && choice != "Feuille" && choice != "Ciseaux") {
                client->send("Choix invalide! Réessayez.\n");
                continue;
            }
            
            // Stocker le choix
            {
                unique_lock<mutex> lock(gameState.mtx);
                if (clientNum == 1) {
                    gameState.choice1 = choice;
                    gameState.choice1Ready = true;
                } else {
                    gameState.choice2 = choice;
                    gameState.choice2Ready = true;
                }
                
                // Attendre que les deux clients aient fait leur choix
                gameState.cv.wait(lock, []{ return gameState.choice1Ready && gameState.choice2Ready; });
            }
            
            // Déterminer le gagnant
            string result = determineWinner(gameState.choice1, gameState.choice2);
            
            string message;
            if (result == "DRAW") {
                message = "Egalite! Joueur1: " + gameState.choice1 + " vs Joueur2: " + gameState.choice2 + "\n";
            } else if ((clientNum == 1 && result == "PLAYER1") || (clientNum == 2 && result == "PLAYER2")) {
                message = "Vous avez gagne! Joueur1: " + gameState.choice1 + " vs Joueur2: " + gameState.choice2 + "\n";
                score++;
            } else {
                message = "Vous avez perdu! Joueur1: " + gameState.choice1 + " vs Joueur2: " + gameState.choice2 + "\n";
            }
            
            message += "Score: " + to_string(score) + "\n";
            message += "Continuer? (O/N): ";
            
            client->send(message);
            
            // Lire la réponse
            msg = "";
            client->read(msg);
            
            if (msg.substr(0, 1) != "O" && msg.substr(0, 1) != "o") {
                client->send("Merci d'avoir joue!\n");
                break;
            }
        }
        
    } catch (const exception& e) {
        cerr << "Erreur: " << e.what() << endl;
    }
    
    delete client;
}

int main() {
    srand(time(nullptr));
    
    ConnectionPoint* server = new ConnectionPoint(3490);
    
    int err = server->init();
    if (err != 0) {
        cerr << "Erreur init serveur: " << strerror(err) << endl;
        exit(err);
    }
    
    cout << "Serveur ecoute sur " << server->getIP() << ":" << server->getPort() << endl;
    
    int clientCount = 0;
    while (clientCount < 2) {
        StreamSocket* client = server->accept();
        
        if (!client->valid()) {
            delete client;
            continue;
        }
        
        clientCount++;
        cout << "Client " << clientCount << " connecte: " << client->getIP() << ":" << client->getPort() << endl;
        
        thread clientThread(handleClient, client, clientCount);
        clientThread.detach();
        
        if (clientCount == 2) {
            {
                unique_lock<mutex> lock(gameState.mtx);
                gameState.cv.notify_all();
            }
        }
    }
    
    delete server;
    return 0;
}
