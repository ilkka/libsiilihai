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
#include "syncmaster.h"
#include "forumsubscription.h"
#include "forumgroup.h"
#include "forumthread.h"
#include "forummessage.h"
#include "forumsubscription.h"

SyncMaster::SyncMaster(QObject *parent, ForumDatabase &fd, SiilihaiProtocol &prot) :
    QObject(parent), fdb(fd), protocol(prot) {

    connect(&protocol, SIGNAL(serverGroupStatus(QList<ForumSubscription*> &)), this,
            SLOT(serverGroupStatus(QList<ForumSubscription*> &)));
    connect(&protocol, SIGNAL(serverThreadData(ForumThread*)), this,
            SLOT(serverThreadData(ForumThread*)));
    connect(&protocol, SIGNAL(serverMessageData(ForumMessage*)), this,
            SLOT(serverMessageData(ForumMessage*)));
    connect(&protocol, SIGNAL(getThreadDataFinished(bool, QString)), this,
            SLOT(getThreadDataFinished(bool, QString)));
    connect(&protocol, SIGNAL(subscribeGroupsFinished(bool)), this, SLOT(subscribeGroupsFinished(bool)));

    canceled = true;
    groupBeingDownloaded = 0;
    errorCount = 0;
}

SyncMaster::~SyncMaster() {
}

void SyncMaster::startSync() {
    canceled = false;
    errorCount = 0;
    groupBeingDownloaded = 0;
    maxGroupCount = 0;
    protocol.getSyncSummary();
}

void SyncMaster::endSync() {
    fdb.checkSanity();
    canceled = false;
    errorCount = 0;

    int totalGroups = 0;
    foreach(ForumSubscription *fsub, fdb.values()) {
        if(fsub->hasGroupListChanged()) {
            forumsToUpload.append(fsub);
            fsub->setGroupListChanged(false);
        }
        foreach(ForumGroup *grp, fsub->values()) {
            if(grp->isSubscribed()) totalGroups++;
            if(grp->isSubscribed() && grp->hasChanged()) {
                groupsToUpload.append(grp);
            }
        }
    }
    maxGroupCount = groupsToUpload.size();
    qDebug() << Q_FUNC_INFO << "Uploading " << maxGroupCount << " of " << totalGroups << " groups.";
    processSubscriptions();
}

void SyncMaster::serverGroupStatus(QList<ForumSubscription*> &subs) { // Temp objects!
    fdb.checkSanity();
    // Update local subs
    foreach(ForumSubscription *serverSub, subs) {
        ForumSubscription *dbSub = fdb.value(serverSub->parser());
        if(!dbSub) { // Whole forum not found in db - add it
            qDebug() << Q_FUNC_INFO << "Forum not in db -  must add it!";
            ForumSubscription *newSub = new ForumSubscription(&fdb, false);
            newSub->copyFrom(serverSub);
            fdb.addSubscription(newSub);
            dbSub = newSub;
        } else { // Sub already in db, just update it
            dbSub->copyFrom(serverSub);
            // Check for unsubscribed groups
            foreach(ForumGroup *dbGrp, dbSub->values()) {
                bool groupIsSubscribed = false;
                foreach(ForumGroup *serverGrp, serverSub->values()) {
                    if(dbGrp->id() == serverGrp->id())
                        groupIsSubscribed = true;
                }
                dbGrp->setSubscribed(groupIsSubscribed);
                dbGrp->commitChanges();
            }
        }
        Q_ASSERT(dbSub);
    }
    // Check for deleted subs
    QQueue<ForumSubscription*> deletedSubs;
    foreach(ForumSubscription *dbSub, fdb.values()) {
        bool found = false;
        foreach(ForumSubscription *serverSub, subs) {
            if(serverSub->parser()==dbSub->parser())
                found = true;
        }
        // Sub in db not found in sync message - delete it
        if(!found) deletedSubs.append(dbSub);
    }
    while(!deletedSubs.isEmpty())
        fdb.deleteSubscription(deletedSubs.takeFirst());

    // Update group lists
    foreach(ForumSubscription *serverSub, subs) {
        foreach(ForumGroup *serverGrp, serverSub->values()) {
            Q_ASSERT(serverGrp->subscription()->parser() >= 0 || serverGrp->id().length() > 0);
            ForumSubscription *dbSub = fdb.value(serverGrp->subscription()->parser());
            Q_ASSERT(dbSub);

            ForumGroup *dbGroup = dbSub->value(serverGrp->id());
            if(!dbGroup) { // Group doesn't exist yet
                qDebug() << Q_FUNC_INFO << "Group " << serverGrp->toString() << " not in db -  must add it!";
                ForumGroup *newGroup = new ForumGroup(dbSub, false);
                serverGrp->setName(UNKNOWN_SUBJECT);
                serverGrp->setChangeset(-1); // Force update of group contents
                serverGrp->markToBeUpdated();
                serverGrp->setSubscribed(true);
                newGroup->copyFrom(serverGrp);
                newGroup->setChangeset(-2); // .. by setting changesets different
                dbSub->addGroup(newGroup);
                dbGroup = newGroup;
            }

            Q_ASSERT(dbGroup);

            if(dbGroup->changeset() != serverGrp->changeset()) {
                qDebug() << Q_FUNC_INFO << "Adding group to download queue and setting changeset " << dbGroup->toString();
                dbGroup->setChangeset(serverGrp->changeset());
                groupsToDownload.append(dbGroup);
            }
            dbGroup->commitChanges();

        }
    }

    processGroups();
}

// Sends the next group in groupsToUpload and download status for
// next in groupsToDownload
void SyncMaster::processGroups() {
    fdb.checkSanity();

    if(canceled) return;
    if (groupsToUpload.isEmpty() && groupsToDownload.isEmpty()) {
        emit syncFinished(true, QString::null);
        return;
    }
    // Do the uploading
    if(!groupsToUpload.isEmpty()) {
        ForumGroup *g = groupsToUpload.takeFirst();
        g->setChangeset(rand());
        foreach(ForumThread *thread, g->values()) {
            Q_ASSERT(thread);
            messagesToUpload.append(thread->values());
        }
        connect(&protocol, SIGNAL(sendThreadDataFinished(bool, QString)), this, SLOT(sendThreadDataFinished(bool, QString)));
        protocol.sendThreadData(g, messagesToUpload);
        g->commitChanges();
        messagesToUpload.clear();
        emit syncProgress(0, "Synchronizing " + g->name() + " in " + g->subscription()->alias() + "..");
    }
    if(!groupsToDownload.isEmpty()) {
        Q_ASSERT(!groupBeingDownloaded);
        groupBeingDownloaded = groupsToDownload.takeFirst();
        protocol.getThreadData(groupBeingDownloaded);
    }
    fdb.checkSanity();
}

void SyncMaster::sendThreadDataFinished(bool success, QString message) {
    qDebug() << Q_FUNC_INFO << success;
    disconnect(&protocol, SIGNAL(sendThreadDataFinished(bool, QString)), this, SLOT(sendThreadDataFinished(bool, QString)));
    if(canceled) return;
    if (success) {
        processGroups();
    } else {
        qDebug() << Q_FUNC_INFO << "Failed! Error count: " << errorCount;
        emit syncFinished(false, message);
    }
}

void SyncMaster::serverThreadData(ForumThread *tempThread) { // Thread is temporary object!
    if(canceled) return;
    if (tempThread->isSane()) {
        ForumThread *dbThread = fdb.getThread(tempThread->group()->subscription()->parser(), tempThread->group()->id(),
                                              tempThread->id());
        if (dbThread) { // Thread already found, merge it
            dbThread->setChangeset(tempThread->changeset());
        } else { // thread hasn't been found yet!
            ForumGroup *dbGroup = fdb.value(tempThread->group()->subscription()->parser())->value(tempThread->group()->id());
            Q_ASSERT(dbGroup);
            Q_ASSERT(!dbGroup->isTemp());
            ForumThread *newThread = new ForumThread(dbGroup, false);
            newThread->copyFrom(tempThread);
            newThread->markToBeUpdated();
            dbGroup->addThread(newThread, false);
            dbThread = newThread;
            // Make sure group will be updated
            dbGroup->markToBeUpdated();
            dbGroup->commitChanges();
        }
        dbThread->commitChanges();
        qDebug() << Q_FUNC_INFO << "Received thread " << dbThread->toString();
        emit syncProgress(0, "Synchronizing " + dbThread->group()->name() + " in " + dbThread->group()->subscription()->alias() + "..");
    } else {
        qDebug() << Q_FUNC_INFO << "Got invalid thread!" << tempThread->toString();
        Q_ASSERT(false);
    }
    fdb.checkSanity();
}

void SyncMaster::serverMessageData(ForumMessage *tempMessage) { // Temporary object!
    qDebug() << Q_FUNC_INFO << "Received message " << tempMessage->toString();
    if(canceled) return;
    if (tempMessage->isSane()) {
        ForumMessage *dbMessage = fdb.getMessage(tempMessage->thread()->group()->subscription()->parser(),
                                                 tempMessage->thread()->group()->id(), tempMessage->thread()->id(), tempMessage->id());
        if (dbMessage) { // Message already found, merge it
            dbMessage->setRead(tempMessage->isRead());
        } else { // message hasn't been found yet!
            qDebug() << Q_FUNC_INFO << "message NOT in DB - adding as new";
            ForumThread *dbThread = fdb.getThread(tempMessage->thread()->group()->subscription()->parser(),
                                                  tempMessage->thread()->group()->id(),
                                                  tempMessage->thread()->id());
            Q_ASSERT(dbThread);
            Q_ASSERT(!dbThread->isTemp());
            ForumMessage *newMessage = new ForumMessage(dbThread, false);
            newMessage->copyFrom(tempMessage);
            newMessage->setRead(true, false);
            dbThread->addMessage(newMessage, false);
            dbThread->setLastPage(0); // Mark as 0 to force update of full thread
            newMessage->markToBeUpdated();
            dbThread->commitChanges();
            dbThread->group()->commitChanges();
        }
    } else {
        qDebug() << Q_FUNC_INFO << "Got invalid message!" << tempMessage->toString();
        Q_ASSERT(false);
    }
    fdb.checkSanity();
    QCoreApplication::processEvents();
}

void SyncMaster::getThreadDataFinished(bool success, QString message){
    if(canceled) return;
    if(success) {
        groupBeingDownloaded = 0;
        processGroups();
    } else {
        errorCount++;
        if(errorCount > 15) {
            groupBeingDownloaded = 0;
            emit syncFinished(false, message);
        } else {
            qDebug() << Q_FUNC_INFO << "Failed! Error count: " << errorCount;
            Q_ASSERT(groupBeingDownloaded);
            groupsToDownload.append(groupBeingDownloaded);
            groupBeingDownloaded = 0;
            processGroups();
        }
    }
}

void SyncMaster::threadChanged(ForumThread *thread) {
}

void SyncMaster::cancel() {
    serversGroups.clear();
    serversThreads.clear();
    groupsToUpload.clear();
    groupsToDownload.clear();
    changedThreads.clear();
    forumsToUpload.clear();
    messagesToUpload.clear();
    emit syncFinished(false, "Canceled");
}

void SyncMaster::processSubscriptions() {
    if(forumsToUpload.isEmpty()) {
        processGroups();
    } else {
        ForumSubscription *sub = forumsToUpload.takeFirst();
        protocol.subscribeGroups(sub);
    }
}

void SyncMaster::subscribeGroupsFinished(bool success) {
    if(canceled) return;
    if (success) {
        processSubscriptions();
    } else {
        qDebug() << Q_FUNC_INFO << "Failed! Error count: " << errorCount;
        emit syncFinished(false, "Updating group subscriptions to server failed");
    }
}
