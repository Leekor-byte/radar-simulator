#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QVector>

struct TargetInfo {
    int id = -1;
    QString name;
    QString type;       // "Ориентир" или "Судно"
    double displacement = 0; // тыс. тонн (только для судна)
    double rcs = 0;         // ЭПР, м²
};

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool initialize();                       // создаёт/открывает БД и таблицу
    bool addTarget(const TargetInfo &target);
    bool removeTarget(int id);
    QVector<TargetInfo> allTargets() const;

private:
    QSqlDatabase m_db;
    bool createTable();
};

#endif // DATABASE_H
