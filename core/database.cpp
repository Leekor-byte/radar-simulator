#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen())
        m_db.close();
}

bool DatabaseManager::initialize()
{
    // SQLite в папке приложения (или в домашней директории)
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/targets.db";
    QDir().mkpath(QFileInfo(dbPath).absolutePath());
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        qWarning() << "Cannot open database:" << m_db.lastError().text();
        return false;
    }
    return createTable();
}

bool DatabaseManager::createTable()
{
    QSqlQuery query(m_db);
    return query.exec(
        "CREATE TABLE IF NOT EXISTS targets ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "type TEXT NOT NULL, "
        "displacement REAL, "
        "rcs REAL NOT NULL)");
}

bool DatabaseManager::addTarget(const TargetInfo &target)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO targets (name, type, displacement, rcs) "
                  "VALUES (:name, :type, :displacement, :rcs)");
    query.bindValue(":name", target.name);
    query.bindValue(":type", target.type);
    query.bindValue(":displacement", target.displacement);
    query.bindValue(":rcs", target.rcs);
    if (!query.exec()) {
        qWarning() << "Insert failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::removeTarget(int id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM targets WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

QVector<TargetInfo> DatabaseManager::allTargets() const
{
    QVector<TargetInfo> result;
    QSqlQuery query("SELECT id, name, type, displacement, rcs FROM targets", m_db);
    while (query.next()) {
        TargetInfo t;
        t.id = query.value(0).toInt();
        t.name = query.value(1).toString();
        t.type = query.value(2).toString();
        t.displacement = query.value(3).toDouble();
        t.rcs = query.value(4).toDouble();
        result.append(t);
    }
    return result;
}
