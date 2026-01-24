#pragma once
#include <QString>
#include <QStringList>
#include <QVariant>

namespace core {
struct IpRange {
    int ipRangeId;
    QString startIp;
    QString stopIp;
    QString startPort;
    QString stopPort;
};
}