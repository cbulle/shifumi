/**
 * shifumiclient.cpp - Implémentation du client Shifumi avec interface Qt
 * @author Etudiant L2 Informatique
 * @version 1.0
 */

#include "shifumiclient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>
#include <iostream>
#include <sstream>

// ===================== ClientWorker =====================
/**
 * Constructeur du worker qui gère la connexion socket
 * @param ip adresse IP du serveur
 * @param port port du serveur
 */
ClientWorker::ClientWorker(const std::string& ip, int port)
    : ip(ip), port(port), socket(nullptr) {
}

/**
 * Destructeur - ferme la socket proprement
 */
ClientWorker::~ClientWorker() {
    if (socket) {
        socket->close();
        delete socket;
    }
}

/**
 * Établit la connexion avec le serveur
 * Appelé dans le thread séparé
 */
void ClientWorker::connect() {
    try {
        socket = new StreamSocket(ip, port);
        int err = socket->connect();
        
        if (err != 0) {
            std::string errMsg = "Erreur de connexion: ";
            errMsg += strerror(err);
            emit connectionError(QString::fromStdString(errMsg));
            return;
        }
        
        std::cout << "Client connecte au serveur" << std::endl;
        emit connectionEstablished();
        
        // Boucle de lecture des messages du serveur
        while (true) {
            std::string msg;
            int bytesRead = socket->read(msg);
            
            if (bytesRead <= 0) {
                std::cout << "Deconnexion du serveur" << std::endl;
                emit disconnected();
                break;
            }
            
            // Émettre le signal avec le message reçu
            emit messageReceived(QString::fromStdString(msg));
        }
        
    } catch (const std::exception& e) {
        std::string errMsg = "Exception: ";
        errMsg += e.what();
        emit connectionError(QString::fromStdString(errMsg));
    }
}

/**
 * Envoie le choix du joueur au serveur
 * @param choice "Pierre", "Feuille" ou "Ciseaux"
 */
void ClientWorker::sendChoice(const std::string& choice) {
    if (socket && socket->valid()) {
        std::string msg = choice + "\r\n";
        int bytesSent = socket->send(msg);
        std::cout << "Choix envoye: " << choice << " (" << bytesSent << " bytes)" << std::endl;
    } else {
        std::cerr << "Socket non valide, impossible d'envoyer le choix" << std::endl;
    }
}

/**
 * Envoie la réponse pour continuer ou arrêter la partie
 * @param continueGame true pour continuer, false pour arrêter
 */
void ClientWorker::sendContinue(bool continueGame) {
    if (socket && socket->valid()) {
        std::string response = continueGame ? "O\r\n" : "N\r\n";
        int bytesSent = socket->send(response);
        std::cout << "Reponse envoyee: " << (continueGame ? "Oui" : "Non") << std::endl;
    } else {
        std::cerr << "Socket non valide, impossible d'envoyer la reponse" << std::endl;
    }
}

// ===================== ShifumiClient =====================
/**
 * Constructeur de la fenêtre principale
 * @param playerNum numéro du joueur (1 ou 2)
 * @param parent widget parent (nullptr par défaut)
 */
ShifumiClient::ShifumiClient(int playerNum, QWidget* parent)
    : QMainWindow(parent), playerNum(playerNum), workerThread(nullptr), worker(nullptr) {
    
    std::cout << "Initialisation du Joueur " << playerNum << std::endl;
    
    setupUI();
    
    // Créer le thread pour la communication socket
    workerThread = new QThread(this);
    worker = new ClientWorker("127.0.0.1", 3490);
    worker->moveToThread(workerThread);
    
    // Connexion des signaux et slots
    connect(workerThread, &QThread::started, worker, &ClientWorker::connect);
    connect(worker, &ClientWorker::messageReceived, this, &ShifumiClient::onMessageReceived);
    connect(worker, &ClientWorker::connectionEstablished, this, &ShifumiClient::onConnectionEstablished);
    connect(worker, &ClientWorker::connectionError, this, &ShifumiClient::onConnectionError);
    connect(this, &ShifumiClient::destroyed, workerThread, &QThread::quit);
    
    // Démarrer le thread
    workerThread->start();
}

/**
 * Destructeur - arrête le thread proprement
 */
ShifumiClient::~ShifumiClient() {
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
        std::cout << "Thread du joueur " << playerNum << " arrete" << std::endl;
    }
}

/**
 * Configure l'interface utilisateur
 * Crée les boutons, labels et zones de texte
 */
void ShifumiClient::setupUI() {
    // Configuration de la fenêtre
    setWindowTitle(QString("Shifumi - Joueur %1").arg(playerNum));
    
    // Positionnement des fenêtres (côte à côte)
    int xPos = 100 + (playerNum - 1) * 450;
    int yPos = 100;
    setGeometry(xPos, yPos, 420, 650);
    
    // Widget central
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Layout principal
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // === TITRE ===
    QLabel* titleLabel = new QLabel(QString("JOUEUR %1").arg(playerNum));
    titleLabel->setStyleSheet(
        "font-size: 20px; "
        "font-weight: bold; "
        "color: #333; "
        "padding: 10px; "
        "background-color: #e0e0e0; "
        "border-radius: 5px; "
        "text-align: center;"
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // === ZONE D'AFFICHAGE ===
    QLabel* displayLabel = new QLabel("Messages du serveur:");
    displayLabel->setStyleSheet("font-weight: bold; color: #333;");
    mainLayout->addWidget(displayLabel);
    
    displayArea = new QTextEdit();
    displayArea->setReadOnly(true);
    displayArea->setStyleSheet(
        "QTextEdit { "
        "background-color: #f9f9f9; "
        "border: 2px solid #ddd; "
        "border-radius: 5px; "
        "padding: 10px; "
        "font-family: Courier; "
        "font-size: 11px; "
        "} "
    );
    displayArea->setMinimumHeight(200);
    mainLayout->addWidget(displayArea);
    
    // === BOUTONS DE CHOIX ===
    QLabel* choiceLabel = new QLabel("Votre choix:");
    choiceLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #333;");
    mainLayout->addWidget(choiceLabel);
    
    QHBoxLayout* choiceLayout = new QHBoxLayout();
    choiceLayout->setSpacing(10);
    
    // Bouton Pierre
    pierreBtn = new QPushButton("PIERRE");
    pierreBtn->setMinimumHeight(60);
    pierreBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #8B7355; "
        "color: white; "
        "padding: 10px; "
        "font-size: 13px; "
        "font-weight: bold; "
        "border: none; "
        "border-radius: 5px; "
        "} "
        "QPushButton:hover { "
        "background-color: #A0826D; "
        "} "
        "QPushButton:pressed { "
        "background-color: #6B5344; "
        "} "
        "QPushButton:disabled { "
        "background-color: #cccccc; "
        "color: #999; "
        "} "
    );
    connect(pierreBtn, &QPushButton::clicked, this, &ShifumiClient::onPierreClicked);
    choiceLayout->addWidget(pierreBtn);
    
    // Bouton Feuille
    feuilleBtn = new QPushButton("FEUILLE");
    feuilleBtn->setMinimumHeight(60);
    feuilleBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #228B22; "
        "color: white; "
        "padding: 10px; "
        "font-size: 13px; "
        "font-weight: bold; "
        "border: none; "
        "border-radius: 5px; "
        "} "
        "QPushButton:hover { "
        "background-color: #2EAE2E; "
        "} "
        "QPushButton:pressed { "
        "background-color: #1A6B1A; "
        "} "
        "QPushButton:disabled { "
        "background-color: #cccccc; "
        "color: #999; "
        "} "
    );
    connect(feuilleBtn, &QPushButton::clicked, this, &ShifumiClient::onFeuilleClicked);
    choiceLayout->addWidget(feuilleBtn);
    
    // Bouton Ciseaux
    ciseauxBtn = new QPushButton("CISEAUX");
    ciseauxBtn->setMinimumHeight(60);
    ciseauxBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #DC143C; "
        "color: white; "
        "padding: 10px; "
        "font-size: 13px; "
        "font-weight: bold; "
        "border: none; "
        "border-radius: 5px; "
        "} "
        "QPushButton:hover { "
        "background-color: #FF1744; "
        "} "
        "QPushButton:pressed { "
        "background-color: #A00D2E; "
        "} "
        "QPushButton:disabled { "
        "background-color: #cccccc; "
        "color: #999; "
        "} "
    );
    connect(ciseauxBtn, &QPushButton::clicked, this, &ShifumiClient::onCiseauxClicked);
    choiceLayout->addWidget(ciseauxBtn);
    
    mainLayout->addLayout(choiceLayout);
    
    // === BOUTONS DE CONTINUATION ===
    QLabel* continueLabel = new QLabel("Continuer la partie?");
    continueLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #333;");
    mainLayout->addWidget(continueLabel);
    
    QHBoxLayout* continueLayout = new QHBoxLayout();
    continueLayout->setSpacing(10);
    
    // Bouton Oui
    ouiBtn = new QPushButton("OUI");
    ouiBtn->setMinimumHeight(50);
    ouiBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #4CAF50; "
        "color: white; "
        "padding: 10px; "
        "font-size: 13px; "
        "font-weight: bold; "
        "border: none; "
        "border-radius: 5px; "
        "} "
        "QPushButton:hover { "
        "background-color: #66BB6A; "
        "} "
        "QPushButton:pressed { "
        "background-color: #2E7D32; "
        "} "
        "QPushButton:disabled { "
        "background-color: #cccccc; "
        "color: #999; "
        "} "
    );
    connect(ouiBtn, &QPushButton::clicked, this, &ShifumiClient::onOuiClicked);
    continueLayout->addWidget(ouiBtn);
    
    // Bouton Non
    nonBtn = new QPushButton("NON");
    nonBtn->setMinimumHeight(50);
    nonBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #f44336; "
        "color: white; "
        "padding: 10px; "
        "font-size: 13px; "
        "font-weight: bold; "
        "border: none; "
        "border-radius: 5px; "
        "} "
        "QPushButton:hover { "
        "background-color: #EF5350; "
        "} "
        "QPushButton:pressed { "
        "background-color: #C62828; "
        "} "
        "QPushButton:disabled { "
        "background-color: #cccccc; "
        "color: #999; "
        "} "
    );
    connect(nonBtn, &QPushButton::clicked, this, &ShifumiClient::onNonClicked);
    continueLayout->addWidget(nonBtn);
    
    mainLayout->addLayout(continueLayout);
    
    // Initialiser l'état des boutons
    enableChoiceButtons(false);
    enableContinueButtons(false);
    
    // Ajouter du stretch pour remplir l'espace vide
    mainLayout->addStretch();
}

/**
 * Active ou désactive les boutons de choix
 * @param enabled true pour activer, false pour désactiver
 */
void ShifumiClient::enableChoiceButtons(bool enabled) {
    pierreBtn->setEnabled(enabled);
    feuilleBtn->setEnabled(enabled);
    ciseauxBtn->setEnabled(enabled);
}

/**
 * Active ou désactive les boutons de continuation
 * @param enabled true pour activer, false pour désactiver
 */
void ShifumiClient::enableContinueButtons(bool enabled) {
    ouiBtn->setEnabled(enabled);
    nonBtn->setEnabled(enabled);
}

/**
 * Slot appelé quand le bouton Pierre est cliqué
 */
void ShifumiClient::onPierreClicked() {
    worker->sendChoice("Pierre");
    enableChoiceButtons(false);
    displayArea->append(">>> Vous avez choisi: PIERRE");
}

/**
 * Slot appelé quand le bouton Feuille est cliqué
 */
void ShifumiClient::onFeuilleClicked() {
    worker->sendChoice("Feuille");
    enableChoiceButtons(false);
    displayArea->append(">>> Vous avez choisi: FEUILLE");
}

/**
 * Slot appelé quand le bouton Ciseaux est cliqué
 */
void ShifumiClient::onCiseauxClicked() {
    worker->sendChoice("Ciseaux");
    enableChoiceButtons(false);
    displayArea->append(">>> Vous avez choisi: CISEAUX");
}

/**
 * Slot appelé quand le bouton Oui est cliqué
 */
void ShifumiClient::onOuiClicked() {
    worker->sendContinue(true);
    enableContinueButtons(false);
    displayArea->append(">>> Vous continuez la partie...");
}

/**
 * Slot appelé quand le bouton Non est cliqué
 */
void ShifumiClient::onNonClicked() {
    worker->sendContinue(false);
    enableContinueButtons(false);
    displayArea->append(">>> Vous avez arrête la partie");
}

/**
 * Slot appelé quand un message est reçu du serveur
 * @param msg le message reçu
 */
void ShifumiClient::onMessageReceived(const QString& msg) {
    displayArea->append(msg);
    
    // Activer les boutons de choix si le serveur demande un choix
    if (msg.contains("choix", Qt::CaseInsensitive) || 
        msg.contains("Pierre", Qt::CaseInsensitive) ||
        msg.contains("Feuille", Qt::CaseInsensitive) ||
        msg.contains("Ciseaux", Qt::CaseInsensitive)) {
        enableChoiceButtons(true);
    }
    
    // Activer les boutons de continuation si le serveur demande de continuer
    if (msg.contains("Continuer", Qt::CaseInsensitive) || 
        msg.contains("O/N", Qt::CaseInsensitive)) {
        enableContinueButtons(true);
    }
}

/**
 * Slot appelé quand la connexion au serveur est établie
 */
void ShifumiClient::onConnectionEstablished() {
    displayArea->append("[✓] Connexion au serveur reussie!");
    displayArea->append("-----------------------------------");
    std::cout << "Joueur " << playerNum << " connecte" << std::endl;
}

/**
 * Slot appelé en cas d'erreur de connexion
 * @param error message d'erreur
 */
void ShifumiClient::onConnectionError(const QString& error) {
    displayArea->append("[✗ ERREUR] " + error);
    enableChoiceButtons(false);
    enableContinueButtons(false);
    std::cerr << "Erreur pour Joueur " << playerNum << ": " << error.toStdString() << std::endl;
}
