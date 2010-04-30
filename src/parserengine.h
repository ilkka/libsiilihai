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
#ifndef PARSERENGINE_H_
#define PARSERENGINE_H_
#include <QObject>
#include "forumparser.h"
#include "forumsubscription.h"
#include "forumsession.h"
#include "forumgroup.h"
#include "forummessage.h"
#include "forumdatabase.h"

class ParserEngine : public QObject {
    Q_OBJECT

public:
    ParserEngine(ForumDatabase *fd, QObject *parent=0);
    virtual ~ParserEngine();
    void setParser(ForumParser &fp);
    void setSubscription(ForumSubscription *fs);
    void updateGroupList();
    void updateForum(bool force=false);
    bool isBusy();

public slots:
    void listMessagesFinished(QList<ForumMessage*> &messages, ForumThread *thread);
    void listGroupsFinished(QList<ForumGroup*> &groups);
    void listThreadsFinished(QList<ForumThread*> &threads, ForumGroup *group);
    void networkFailure(QString message);
    void cancelOperation();

signals:
    // Emitted if initially group list was empty but new groups
    // were found.
    void groupListChanged(ForumSubscription *forum);
    void forumUpdated(ForumSubscription *forum);
    void statusChanged(ForumSubscription *forum, bool reloading, float progress);
    void updateFailure(ForumSubscription *forum, QString message);
    void getAuthentication(ForumSubscription *fsub, QAuthenticator *authenticator);

private:
    void updateNextChangedGroup();
    void updateNextChangedThread();
    void setBusy(bool busy);
    void updateCurrentProgress();
    ForumParser parser;
    ForumSubscription *subscription;
    ForumSession session;
    bool sessionInitialized;
    bool updateAll;
    bool forumBusy;
    bool forceUpdate; // Update even if no changes
    ForumDatabase *fdb;
    QQueue<ForumGroup*> groupsToUpdateQueue;
    QQueue<ForumThread*> threadsToUpdateQueue;
};

#endif /* PARSERENGINE_H_ */
