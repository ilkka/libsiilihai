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
#ifndef FORUMSESSION_H_
#define FORUMSESSION_H_
#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QNetworkCookieJar>
#include <QDebug>
#include <QAuthenticator>

#include "forumparser.h"
#include "forumsubscription.h"
#include "httppost.h"
#include "patternmatcher.h"
#include "forumgroup.h"
#include "forumthread.h"
#include "forummessage.h"

class ForumSession : public QObject {
    Q_OBJECT
public:
    enum ForumSessionOperation { FSONoOp, FSOListGroups, FSOUpdateThreads, FSOUpdateMessages };

    ForumSession(QObject *parent=0);
    virtual ~ForumSession();
    void initialize(ForumParser &fop, ForumSubscription *fos, PatternMatcher *matcher=0);
    void clearAuthentications();
    void setParser(ForumParser &fop);
    void listGroups();
    void listThreads(ForumGroup *group);
    void listMessages(ForumThread *thread);
    void loginToForum();


    QString getMessageUrl(const ForumMessage *msg);
    QString getLoginUrl();
    QString getThreadListUrl(const ForumGroup *grp, int page=-1);
    QString getMessageListUrl(const ForumThread *thread, int page=-1);

    void performListGroups(QString &html);
    void performListThreads(QString &html);
    void performListMessages(QString &html);
    void performLogin(QString &html);

public slots:
    void listGroupsReply(QNetworkReply *reply);
    void listThreadsReply(QNetworkReply *reply);
    void listMessagesReply(QNetworkReply *reply);
    void fetchCookieReply(QNetworkReply *reply);
    void loginReply(QNetworkReply *reply);
    void cancelOperation();
    void authenticationRequired ( QNetworkReply * reply, QAuthenticator * authenticator );

signals:
    void listGroupsFinished(QList<ForumGroup*> &groups);
    void listThreadsFinished(QList<ForumThread*> &threads, ForumGroup *group);
    void listMessagesFinished(QList<ForumMessage*> &messages, ForumThread *thread);
    void networkFailure(QString message);
    void loginFinished(bool success);
    void receivedHtml(const QString &data);
    void getAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator);

private:
    bool prepareForUse(); // get cookie & login if needed
    void nextOperation();
    void fetchCookie();
    void updateGroupPage();
    void updateThreadPage();
    QString convertCharset(const QByteArray &src);
    QString statusReport();
    PatternMatcher *pm;
    ForumParser fpar;
    ForumSubscription *fsub;
    QNetworkAccessManager *nam;
    QByteArray emptyData, loginData;
    bool cookieFetched, loggedIn;
    ForumSessionOperation operationInProgress;
    QNetworkCookieJar *cookieJar;
    int currentListPage;
    ForumGroup *currentGroup;
    ForumThread *currentThread;
    QList<ForumThread*> threads;
    QList<ForumMessage*> messages; // Represents messages in thread listMessages
    QString currentMessagesUrl;
};

#endif /* FORUMSESSION_H_ */
