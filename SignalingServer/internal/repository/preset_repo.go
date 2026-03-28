package repository

import (
	"quickdesk/signaling/internal/models"

	"gorm.io/gorm"
)

type PresetRepository struct {
	db *gorm.DB
}

func NewPresetRepository(db *gorm.DB) *PresetRepository {
	return &PresetRepository{db: db}
}

// GetPreset returns the single preset row, creating a default one if none exists.
func (r *PresetRepository) GetPreset() (*models.Preset, error) {
	var preset models.Preset
	result := r.db.First(&preset)
	if result.Error != nil {
		if result.Error == gorm.ErrRecordNotFound {
			preset = models.Preset{
				Notice:     "",
				Links:      "",
				MinVersion: "",
			}
			if err := r.db.Create(&preset).Error; err != nil {
				return nil, err
			}
			return &preset, nil
		}
		return nil, result.Error
	}
	return &preset, nil
}

// UpsertPreset updates the single preset row, or creates it if not exists.
func (r *PresetRepository) UpsertPreset(preset *models.Preset) error {
	var existing models.Preset
	result := r.db.First(&existing)
	if result.Error != nil {
		if result.Error == gorm.ErrRecordNotFound {
			return r.db.Create(preset).Error
		}
		return result.Error
	}
	existing.Notice = preset.Notice
	existing.Links = preset.Links
	existing.MinVersion = preset.MinVersion
	existing.WebclientURL = preset.WebclientURL
	return r.db.Save(&existing).Error
}
