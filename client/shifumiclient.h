/**
 * shifumiclient.h - Interface graphique pour le client Shifumi
 */
#ifndef SHIFUMICLIENT_H
#define SHIFUMICLIENT_H

#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QThread>
#include <memory>
#include "socket.h"

using namespace stdsock;

class ClientWorker : public QObject {
    Q_OBJECT

public:
    ClientWorker(const std::string& ip, int port);
    ~ClientWorker();

public slots:
    void connect();
    void sendChoice(const std::string& choice);
    void sendContinue(bool continueGame);

signals:
    void messageReceived(const QString& msg);
    void connectionEstablished();
    void connectionError(const QString& error);
    void disconnected();

private:
    std::string ip;
    int port;
    StreamSocket* socket;
};

class ShifumiClient : public QMainWindow {
    Q_OBJECT

public:
    ShifumiClient(int playerNum, QWidget* parent = nullptr);
    ~ShifumiClient();

private slots:
    void onPierreClicked();
    void onFeuilleClicked();
    void onCiseauxClicked();
    void onOuiClicked();
    void onNonClicked();
    void onMessageReceived(const QString& msg);
    void onConnectionEstablished();
    void onConnectionError(const QString& error);

private:
    void setupUI();
    void enableChoiceButtons(bool enabled);
    void enableContinueButtons(bool enabled);

    int playerNum;
    QTextEdit* displayArea;
    QPushButton* pierreBtn;
    QPushButton* feuilleBtn;
    QPushButton* ciseauxBtn;
    QPushButton* ouiBtn;
    QPushButton* nonBtn;
    
    QThread* workerThread;
    ClientWorker* worker;
};

#endif
