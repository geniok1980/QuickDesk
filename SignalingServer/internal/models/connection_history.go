package models

import "time"

// ConnectionHistory records every successful or failed connection attempt
// made by a user to a device.
type ConnectionHistory struct {
	ID         uint      `gorm:"primaryKey" json:"id"`
	UserID     uint      `gorm:"not null;index" json:"user_id"`
	DeviceID   string    `gorm:"size:9;not null;index" json:"device_id"`
	DeviceName string    `gorm:"size:100" json:"device_name"`
	ConnectIP  string    `gorm:"size:50" json:"connect_ip"`
	Duration   int       `json:"duration"` // seconds
	Status     string    `gorm:"size:20" json:"status"` // success / failed / timeout
	ErrorMsg   string    `gorm:"size:255" json:"error_msg"`
	CreatedAt  time.Time `json:"created_at"`

	// BelongsTo
	User User `gorm:"foreignKey:UserID" json:"user,omitempty"`
}

// TableName overrides the default table name.
func (ConnectionHistory) TableName() string {
	return "connection_histories"
}
