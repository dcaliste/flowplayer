#include "mydatabase.h"

#include <QDebug>

extern bool isDBOpened;
extern bool databaseWorking;

void openDatabase()
{
    if (database.isOpen())
        database.close();

    qDebug() << "DATABASES: " << database.connectionNames().join(" - ");

    if (!database.connectionNames().join("-").contains("flowplayer")) {
        database = QSqlDatabase::addDatabase("QSQLITE", "flowplayer");
        QString path("/home/nemo/.config/cepiperez/flowplayer.db");
        qDebug() << "Database: " << path;
        database.setDatabaseName(path);
    }

    if (database.open()) {
        isDBOpened = true;
        executeQuery("alter table tracks add fav integer");
    }
}

QSqlQuery getQuery(QString qr)
{
    if (database.open())
        isDBOpened = true;

    databaseWorking = true;
    QSqlQuery query(database);
    query.exec(qr);

    databaseWorking = false;
    return query;
}

bool executeQuery(QString qr)
{
    if (database.open())
        isDBOpened = true;

    databaseWorking = true;
    QSqlQuery *query = new QSqlQuery(database);
    bool res = query->exec(qr);
    delete query;

    databaseWorking = false;
    return res;
}

bool executeQueryCheckCount(QString qr)
{
    if (database.open())
        isDBOpened = true;

    databaseWorking = true;
    QSqlQuery *query = new QSqlQuery(database);
    query->exec(qr);
    bool res;
    if (query->next())
        res = true;
    else
        res = false;

    databaseWorking = false;
    return res;
    delete query;

}

