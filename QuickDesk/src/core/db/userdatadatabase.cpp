#include "userdatadatabase.h"

#include <QDataStream>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "infra/env/applicationcontext.h"
#include "infra/log/log.h"

namespace core {

constexpr char kDeviceDb[] = "userdata.db";
constexpr char kIpRangeListTable[] = "ip_range_list";

bool UserDataDataBase::init()
{
    return DBBase::init(infra::ApplicationContext::instance().dbPath(), kDeviceDb);
}

bool UserDataDataBase::initTables()
{
    QSqlQuery query(QSqlDatabase::database(kDeviceDb));
    // 注意：表名和字段名不能用占位符+bindValue的方式指定，sql为了防止sql注入设计的
    QString sql = QString(R"(
        CREATE TABLE IF NOT EXISTS %1 (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            start_ip TEXT NOT NULL,
            stop_ip TEXT NOT NULL,
            port TEXT NOT NULL,
            stop_port TEXT NOT NULL
            )
        )")
              .arg(kIpRangeListTable);
    if (!query.prepare(sql) || !query.exec()) {
        LOG_ERROR("[database] create table {} failed:{}", kIpRangeListTable, query.lastError().text().toStdString());
        return false;
    }

    if (!upgradeTables()) {
        return false;
    }

    return true;
}

bool UserDataDataBase::upgradeTables()
{
    return true;
}

int UserDataDataBase::getLastId(const QString& tableName)
{
    QString sql = QString("SELECT seq FROM sqlite_sequence WHERE name='%1'").arg(tableName);
    QSqlQuery query(QSqlDatabase::database(kDeviceDb));
    query.prepare(sql);
    if (!query.exec()) {
        LOG_ERROR("[database] select table {} failed:{}", tableName.toStdString(), query.lastError().text().toStdString());
        return 0;
    }

    int lastId = 0;
    while (query.next()) {
        lastId = query.value(0).toInt();
        return lastId;
    }

    return lastId;
}

bool UserDataDataBase::addIpRange(const IpRange& ipRange, int& ipRangeId)
{
    QString sql = QString("INSERT INTO %1 (start_ip, stop_ip, port, stop_port) VALUES(:start_ip, :stop_ip, :port, :stop_port)").arg(kIpRangeListTable);
    QSqlQuery query(QSqlDatabase::database(kDeviceDb));
    query.prepare(sql);
    query.bindValue(":start_ip", ipRange.startIp);
    query.bindValue(":stop_ip", ipRange.stopIp);
    query.bindValue(":port", ipRange.startPort);
    query.bindValue(":stop_port", ipRange.stopPort);
    if (!query.exec()) {
        LOG_ERROR("[database] insert into table {} failed:{}", kIpRangeListTable, query.lastError().text().toStdString());
        return false;
    }
    query.finish();

    ipRangeId = getLastId(kIpRangeListTable);
    return true;
}

bool UserDataDataBase::removeIpRange(int ipRangeId)
{
    QString sql = QString("DELETE FROM %1 WHERE id=:id").arg(kIpRangeListTable);
    QSqlQuery query(QSqlDatabase::database(kDeviceDb));
    query.prepare(sql);
    query.bindValue(":id", ipRangeId);
    if (!query.exec()) {
        LOG_ERROR("[database] delete table {} failed:{}", kIpRangeListTable, query.lastError().text().toStdString());
        return false;
    }

    return true;
}

bool UserDataDataBase::allIpRange(QVector<IpRange>& ipRanges)
{
    QString sql = QString("SELECT id, start_ip, stop_ip, port, stop_port FROM %1").arg(kIpRangeListTable);
    QSqlQuery query(QSqlDatabase::database(kDeviceDb));
    query.prepare(sql);
    if (!query.exec()) {
        LOG_ERROR("[database] select table {} failed:{}", kIpRangeListTable, query.lastError().text().toStdString());
        return false;
    }

    while (query.next()) {
        int id = query.value(0).toInt();
        QString startIp = query.value(1).toString();
        QString stopIp = query.value(2).toString();
        QString startPort = query.value(3).toString();
        QString stopPort = query.value(4).toString();
        // 兼容旧版本
        if (stopPort.isEmpty()) {
            stopPort = startPort;
        }
        ipRanges.push_back({ id, startIp, stopIp, startPort, stopPort });
    }

    return true;
}
} // namespace core
