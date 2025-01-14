#ifndef FORUMDATABASEXML_H
#define FORUMDATABASEXML_H

#include <QIODevice>
#include <QString>
#include <QFile>
#include "forumdatabase.h"

class ForumDatabaseXml : public ForumDatabase
{
    Q_OBJECT
public:
    explicit ForumDatabaseXml(QObject *parent = 0);
    bool openDatabase(QIODevice *source);
    bool openDatabase(QString filename);

    virtual void resetDatabase();
    virtual int schemaVersion();
    virtual bool isStored();

    // Subscription related
    virtual bool addSubscription(ForumSubscription *fs); // Ownership changes!!!
    virtual void deleteSubscription(ForumSubscription *sub);

public slots:
    virtual bool storeDatabase();

signals:
    void subscriptionFound(ForumSubscription *sub);
    void subscriptionRemoved(ForumSubscription *sub);
    void databaseStored();
private:
    bool unsaved, loaded;
    QString databaseFileName;
};

#endif // FORUMDATABASEXML_H
