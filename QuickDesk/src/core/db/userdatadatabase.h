#pragma once

#include <QByteArray>
#include <QPair>
#include <QString>
#include <QVariant>

#include "core/common/commontype.h"
#include "infra/db/dbbase.h"

namespace core {

class UserDataDataBase : public infra::DBBase {
public:
    virtual ~UserDataDataBase() = default;

    bool init();

    bool addIpRange(const IpRange& ipRange, int& ipRangeId);
    bool removeIpRange(int ipRangeId);
    bool allIpRange(QVector<IpRange>& ipRanges);

private:
    bool initTables() override;
    bool upgradeTables();
    int getLastId(const QString& tableName);
};

}
