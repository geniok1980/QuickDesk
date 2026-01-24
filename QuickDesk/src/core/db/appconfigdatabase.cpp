#include "appconfigdatabase.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "infra/env/applicationcontext.h"
#include "infra/log/log.h"

namespace core {

constexpr char kAppConfigDb[] = "appconfig.db";
constexpr char kAppConfigKeyValueNewTable[] = "key_value_new";

bool AppConfigDataBase::init()
{
    return infra::DBBase::init(infra::ApplicationContext::instance().dbPath(), kAppConfigDb);
}

bool AppConfigDataBase::initTables()
{
    QSqlQuery query(QSqlDatabase::database(kAppConfigDb));
    // 注意：表名和字段名不能用占位符+bindValue的方式指定，sql为了防止sql注入设计的
    QString sql = QString("CREATE TABLE IF NOT EXISTS %1 (name TEXT PRIMARY KEY UNIQUE NOT NULL, value TEXT)").arg(kAppConfigKeyValueNewTable);
    if (!query.prepare(sql) || !query.exec()) {
        LOG_ERROR("create table {} failed:{}", kAppConfigKeyValueNewTable, query.lastError().text().toStdString());
        return false;
    }

    bool exitOld = false;
    {
        // 判断kAppConfigKeyValueTable表是否存在
        QSqlQuery queryOld("SELECT * FROM key_value", QSqlDatabase::database(kAppConfigDb));
        exitOld = queryOld.next();
    }
    if (exitOld) {
        // 复制旧表到新表然后删除
        QSqlQuery insert(QSqlDatabase::database(kAppConfigDb));
        insert.prepare("INSERT INTO key_value_new SELECT * FROM key_value");
        insert.exec();
        QSqlQuery drop(QSqlDatabase::database(kAppConfigDb));
        drop.prepare("DROP TABLE key_value");
        drop.exec();
    }

    return true;
}

QString AppConfigDataBase::get(const QString& key)
{
    // QSqlQuery构造函数直接传递sql会被立即执行，所以sql中需要构造好参数
    // 想用bindValue构造参数的话就得配合prepare+exec使用
#if 0
    QString sql = QString("SELECT value FROM %1 WHERE name=:name").arg(kAppConfigKeyValueNewTable);
    QSqlQuery query(QSqlDatabase::database(kAppConfigDb));
    query.prepare(sql);
    query.bindValue(":name", key);
    query.exec();
    while (query.next()) {
        return query.value(0).toString();
    }
#else
    QString sql = QString("SELECT value FROM %1 WHERE name='%2'").arg(kAppConfigKeyValueNewTable, key);
    // 构造QSqlQuery时传递了sql，则内部立即执行
    QSqlQuery query(sql, QSqlDatabase::database(kAppConfigDb));
    while (query.next()) {
        return query.value(0).toString();
    }
#endif

    return "";
}

bool AppConfigDataBase::set(const QString& key, const QString& value)
{
    QString sql = QString("REPLACE INTO %1 (name, value) VALUES(:name, :value)").arg(kAppConfigKeyValueNewTable);
    QSqlQuery query(QSqlDatabase::database(kAppConfigDb));
    query.prepare(sql);
    query.bindValue(":name", key);
    query.bindValue(":value", value);
    if (!query.exec()) {
        LOG_ERROR("replace into table {} failed:{}", kAppConfigKeyValueNewTable, query.lastError().text().toStdString());
        return false;
    }

    return true;
}

}
