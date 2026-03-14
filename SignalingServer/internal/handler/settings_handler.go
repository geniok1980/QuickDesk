package handler

import (
	"net/http"
	"quickdesk/signaling/internal/models"

	"github.com/gin-gonic/gin"
	"gorm.io/gorm"
)

// SettingsHandler handles system settings read/write operations.
type SettingsHandler struct {
	db *gorm.DB
}

// NewSettingsHandler creates a new SettingsHandler.
func NewSettingsHandler(db *gorm.DB) *SettingsHandler {
	return &SettingsHandler{db: db}
}

// GetSettings handles GET /api/v1/settings and GET /admin/settings
// Returns the single system settings row, or defaults if not yet configured.
func (h *SettingsHandler) GetSettings(c *gin.Context) {
	var settings models.Settings
	result := h.db.First(&settings)
	if result.Error != nil {
		if result.Error == gorm.ErrRecordNotFound {
			c.JSON(http.StatusOK, gin.H{
				"siteEnabled": true,
				"siteName":    "QuickDesk",
				"loginLogo":   "",
				"smallLogo":   "",
				"favicon":     "",
			})
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{"error": result.Error.Error()})
		return
	}
	c.JSON(http.StatusOK, settings)
}

// UpdateSettings handles POST /admin/settings
// Creates the settings row on first call; updates it on subsequent calls.
func (h *SettingsHandler) UpdateSettings(c *gin.Context) {
	var settings models.Settings
	if err := c.ShouldBindJSON(&settings); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	var existing models.Settings
	result := h.db.First(&existing)
	if result.Error != nil && result.Error != gorm.ErrRecordNotFound {
		c.JSON(http.StatusInternalServerError, gin.H{"error": result.Error.Error()})
		return
	}

	if result.Error == gorm.ErrRecordNotFound {
		if err := h.db.Create(&settings).Error; err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
			return
		}
	} else {
		settings.ID = existing.ID
		if err := h.db.Save(&settings).Error; err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
			return
		}
	}

	c.JSON(http.StatusOK, settings)
}
