/* This file is part of libSiilihai.

    libSiilihai is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libSiilihai is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libSiilihai.  If not, see <http://www.gnu.org/licenses/>. */
#ifndef SIILIHAIPROTOCOL_H_
#define SIILIHAIPROTOCOL_H_
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QNetworkCookieJar>
#include <QCoreApplication>
#include <QDomDocument>
#include <QDebug>
#include <QHash>
#include <QList>

class ForumParser;
class UserSettings;
class ParserReport;
class ForumRequest;
class ForumSubscription;
class ForumGroup;
class ForumThread;
class ForumMessage;

#define CLIENT_VERSION "1.0.0"

/**
  * Implements communication protocol between siilihai.com server and the client.
  *
  * @todo https
  */
class SiilihaiProtocol: public QObject {
    Q_OBJECT
public:
    enum SiilihaiProtocolOperation { SPONoOp=1, SPOLogin, SPORegisterUser, SPOListParsers, SPOListRequests, SPOListSubscriptions,
                                   SPOGetParser, SPOSubscribeForum, SPOSubscribeGroups, SPOSaveParser, SPOSetUserSettings,
                                   SPOGetUserSettings, SPOSendParserReport, SPOSendThreadData, SPOGetThreadData, SPOGetSyncSummary};

    SiilihaiProtocol(QObject *parent = 0);
    virtual ~SiilihaiProtocol();
    void login(QString user, QString pass);
    void registerUser(QString user, QString pass, QString email, bool sync);
    void setBaseURL(QString bu);
    QString baseURL();
    void listParsers();
    void listRequests();
    void listSubscriptions();
    void getParser(const int id);
    void subscribeGroups(ForumSubscription *fs); // Sends groups in forum to server
    void saveParser(const ForumParser *parser);

    void setUserSettings(UserSettings *us);
    void getUserSettings();

    // Messages must be in same group!
    void sendThreadData(ForumGroup *grp, QList<ForumMessage*> &fms);
    void getSyncSummary();
    void getThreadData(ForumGroup *grp);

    bool isLoggedIn();
public slots:
    void subscribeForum(ForumSubscription *fs, bool unsubscribe = false);
    void sendParserReport(ParserReport *pr);

private slots:
    void networkReply(QNetworkReply *reply);
private:
    void replyLogin(QNetworkReply *reply);
    void replyListParsers(QNetworkReply *reply);
    void replyListRequests(QNetworkReply *reply);
    void replyGetParser(QNetworkReply *reply);
    void replySaveParser(QNetworkReply *reply);
    void replySubscribeForum(QNetworkReply *reply);
    void replyListSubscriptions(QNetworkReply *reply);
    void replySendParserReport(QNetworkReply *reply);
    void replySubscribeGroups(QNetworkReply *reply);
    void replySendThreadData(QNetworkReply *reply);
    void replyGetSyncSummary(QNetworkReply *reply);
    void replyGetThreadData(QNetworkReply *reply);
    void replyGetUserSettings(QNetworkReply *reply);


signals:
    void loginFinished(bool success, QString motd, bool syncEnabled);
    void listParsersFinished(QList<ForumParser*> parsers); // Receiver MUST free the parsers!
    void listRequestsFinished(QList<ForumRequest*> requests); // Receiver MUST free requests!
    void subscribeForumFinished(ForumSubscription *fs, bool success);
    void getParserFinished(ForumParser *parser); // Parser is deleted after call!
    void saveParserFinished(int newId, QString message);
    void listSubscriptionsFinished(QList<int> subscriptions);
    void sendParserReportFinished(bool success);
    void subscribeGroupsFinished(bool success);

    void userSettingsReceived(bool success, UserSettings *newSettings);

    // Sync stuff:
    void sendThreadDataFinished(bool success, QString message);
    void serverGroupStatus(QList<ForumSubscription*> &subs); // Temp objects!
    void serverThreadData(ForumThread *thread); // Temporary object, don't store!
    void serverMessageData(ForumMessage *message); // Temporary object, don't store!
    void getThreadDataFinished(bool success, QString message);

private:
    QString clientKey;
    QNetworkAccessManager nam;
    QString baseUrl;
    QByteArray loginData, listParsersData, saveParserData, getParserData,
    subscribeForumData, listRequestsData, registerData, listSubscriptionsData,
    sendParserReportData, subscribeGroupsData, sendThreadDataData, getThreadDataData,
    syncSummaryData, userSettingsData;
    QUrl listParsersUrl, loginUrl, getParserUrl, saveParserUrl,
    subscribeForumUrl, listRequestsUrl, registerUrl, listSubscriptionsUrl,
    sendParserReportUrl, subscribeGroupsUrl, sendThreadDataUrl, getThreadDataUrl, syncSummaryUrl,
    userSettingsUrl;
    ForumGroup *getThreadDataGroup;
    ForumSubscription *forumBeingSubscribed;
    SiilihaiProtocolOperation operationInProgress;
};

#endif /* SIILIHAIPROTOCOL_H_ */
