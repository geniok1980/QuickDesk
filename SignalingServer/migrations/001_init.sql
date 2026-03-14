-- Reference SQL for QuickDesk Signaling Server
-- NOTE: Tables are auto-created by GORM AutoMigrate on server startup.
--       This file serves as documentation only.

-- Create devices table
CREATE TABLE IF NOT EXISTS devices (
    id SERIAL PRIMARY KEY,
    device_id VARCHAR(9) UNIQUE NOT NULL,
    device_uuid UUID UNIQUE NOT NULL,
    os VARCHAR(50),
    os_version VARCHAR(50),
    app_version VARCHAR(20),
    user_id INTEGER DEFAULT 0,         -- bound user ID (0 = unbound)
    device_name VARCHAR(100),          -- user-defined device name
    remark VARCHAR(255),               -- device remark
    access_code VARCHAR(6),            -- 6-digit access code
    online BOOLEAN DEFAULT false,
    last_seen TIMESTAMP,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_device_id ON devices(device_id);
CREATE INDEX IF NOT EXISTS idx_device_uuid ON devices(device_uuid);
CREATE INDEX IF NOT EXISTS idx_devices_user_id ON devices(user_id);
CREATE INDEX IF NOT EXISTS idx_online ON devices(online);
CREATE INDEX IF NOT EXISTS idx_last_seen ON devices(last_seen);

COMMENT ON TABLE devices IS 'QuickDesk host devices';
COMMENT ON COLUMN devices.device_id IS '9-digit unique device identifier';
COMMENT ON COLUMN devices.device_uuid IS 'UUID for internal use';
COMMENT ON COLUMN devices.user_id IS 'bound user ID, 0 means unbound';
COMMENT ON COLUMN devices.device_name IS 'user-defined device name';
COMMENT ON COLUMN devices.remark IS 'device remark/description';
COMMENT ON COLUMN devices.access_code IS '6-digit numeric access code';
COMMENT ON COLUMN devices.online IS 'Whether the device is currently online';
COMMENT ON COLUMN devices.last_seen IS 'Last time the device was seen online';

-- Create presets table (server preset configuration)
CREATE TABLE IF NOT EXISTS presets (
    id SERIAL PRIMARY KEY,
    notice TEXT DEFAULT '',
    links TEXT DEFAULT '',
    min_version VARCHAR(20) DEFAULT '',
    updated_at TIMESTAMP DEFAULT NOW()
);

COMMENT ON TABLE presets IS 'Server preset configuration (single row)';
COMMENT ON COLUMN presets.notice IS 'Announcement text, JSON: {"zh_CN":"...", "en_US":"..."}';
COMMENT ON COLUMN presets.links IS 'Navigation links, JSON: {"zh_CN":[{icon,text,url},...], "en_US":[...]}';
COMMENT ON COLUMN presets.min_version IS 'Minimum allowed client version, e.g. "1.0.0"';

-- Create admin_users table (administrator accounts)
CREATE TABLE IF NOT EXISTS admin_users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password TEXT NOT NULL,   -- bcrypt hash
    email VARCHAR(100),
    role VARCHAR(20) DEFAULT 'admin',  -- admin, super_admin
    status BOOLEAN DEFAULT true,
    last_login TIMESTAMP,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_admin_username ON admin_users(username);
CREATE INDEX IF NOT EXISTS idx_admin_email ON admin_users(email);

COMMENT ON TABLE admin_users IS 'Administrator accounts (bcrypt passwords)';
COMMENT ON COLUMN admin_users.role IS 'admin or super_admin';
COMMENT ON COLUMN admin_users.status IS 'true: active, false: disabled';
