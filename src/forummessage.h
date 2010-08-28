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
#ifndef FORUMMESSAGE_H_
#define FORUMMESSAGE_H_
#include <QString>
#include <QObject>
#include "forumthread.h"

class ForumMessage : public QObject {
    Q_OBJECT
public:
    virtual ~ForumMessage();
    ForumMessage(ForumThread *thr, bool temp=true);
    void copyFrom(ForumMessage * o);
    bool isSane() const;
    QString toString() const;
    ForumThread* thread() const;
    QString id() const;
    int ordernum() const;
    QString url() const;
    QString subject() const;
    QString author() const;
    QString lastchange() const;
    QString body() const;
    bool read() const;

    void setId(QString nid);
    void setOrdernum(int nod);
    void setUrl(QString nurl);
    void setSubject(QString ns);
    void setAuthor(QString na);
    void setLastchange(QString nlc);
    void setBody(QString nb);
    void setRead(bool nr);
//    void setThread(ForumThread *nt);
    bool isTemp();

signals:
    void changed(ForumMessage * fm);
    void markedRead(ForumMessage * fm, bool read);

private:
    Q_DISABLE_COPY(ForumMessage);
    QString _id;
    int _ordernum;
    QString _url;
    QString _subject;
    QString _author;
    QString _lastchange;
    QString _body;
    bool _read;
    ForumThread *_thread;
    bool _temp;
};

#endif /* FORUMMESSAGE_H_ */
