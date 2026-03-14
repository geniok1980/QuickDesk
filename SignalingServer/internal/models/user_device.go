package models

import "time"

// UserDevice records the binding relationship between a user and a device.
// A binding is created when the user successfully connects to a device.
type UserDevice struct {
	ID           uint      `gorm:"primaryKey" json:"id"`
	UserID       uint      `gorm:"not null;index" json:"user_id"`
	DeviceID     string    `gorm:"size:9;not null;index" json:"device_id"`
	DeviceName   string    `gorm:"size:100" json:"device_name"`                    // user-defined label
	BindType     string    `gorm:"size:20;default:'manual'" json:"bind_type"`      // manual / auto
	Status       bool      `gorm:"default:true" json:"status"`                     // true=active, false=unbound
	LastConnect  time.Time `json:"last_connect"`
	ConnectCount int       `gorm:"default:0" json:"connect_count"`
	CreatedAt    time.Time `json:"created_at"`
	UpdatedAt    time.Time `json:"updated_at"`

	// BelongsTo
	User User `gorm:"foreignKey:UserID" json:"user,omitempty"`
}

// TableName overrides the default table name.
func (UserDevice) TableName() string {
	return "user_devices"
}
