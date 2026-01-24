#pragma once

#include <QObject>
#include <QString>

#include "base/singleton.h"
#include "core/common/commontype.h"

namespace core {

class UserDataDataBase;
class UserDataCenter : public QObject, public base::Singleton<UserDataCenter> {
    Q_OBJECT
public:
    UserDataCenter(QObject* parent = nullptr);
    ~UserDataCenter();

    bool init();

    bool addIpRange(const IpRange& ipRange, int& ipRangeId);
    bool removeIpRange(int ipRangeId);
    bool allIpRange(QVector<IpRange>& ipRanges);

private:
    UserDataDataBase* m_userDataDB = nullptr;
};

}
