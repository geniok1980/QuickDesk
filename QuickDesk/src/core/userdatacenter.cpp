#include "userdatacenter.h"

#include "core/db/userdatadatabase.h"

namespace core {

UserDataCenter::UserDataCenter(QObject* parent)
    : QObject(parent)
{
    m_userDataDB = new UserDataDataBase();
}

UserDataCenter::~UserDataCenter()
{
    delete m_userDataDB;
    m_userDataDB = nullptr;
}

bool UserDataCenter::init()
{
    return m_userDataDB->init();
}

bool UserDataCenter::addIpRange(const IpRange& ipRange, int& ipRangeId)
{
    return m_userDataDB->addIpRange(ipRange, ipRangeId);
}

bool UserDataCenter::removeIpRange(int ipRangeId)
{
    return m_userDataDB->removeIpRange(ipRangeId);
}

bool UserDataCenter::allIpRange(QVector<IpRange>& ipRanges)
{
    return m_userDataDB->allIpRange(ipRanges);
}

}
