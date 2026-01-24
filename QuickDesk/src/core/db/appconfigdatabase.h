#pragma once

#include <QString>

#include "infra/db/dbbase.h"

namespace core {

class AppConfigDataBase : public infra::DBBase {
public:
    virtual ~AppConfigDataBase() = default;

    bool init();
    QString get(const QString& key);
    bool set(const QString& key, const QString& value);

private:
    bool initTables() override;
};

}
