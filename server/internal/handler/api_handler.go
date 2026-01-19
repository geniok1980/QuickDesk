package handler

import (
	"fmt"
	"log"
	"net/http"
	"quickdesk/signaling/internal/config"
	"quickdesk/signaling/internal/service"

	"github.com/gin-gonic/gin"
)

type APIHandler struct {
	deviceService *service.DeviceService
	authService   *service.AuthService
	config        *config.Config
}

func NewAPIHandler(deviceService *service.DeviceService, authService *service.AuthService, cfg *config.Config) *APIHandler {
	return &APIHandler{
		deviceService: deviceService,
		authService:   authService,
		config:        cfg,
	}
}

// RegisterDevice handles POST /api/v1/devices/register
func (h *APIHandler) RegisterDevice(c *gin.Context) {
	var req service.RegisterDeviceRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid request"})
		return
	}
	
	device, err := h.deviceService.RegisterDevice(c.Request.Context(), &req)
	if err != nil {
		log.Printf("Failed to register device: %v", err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to register device"})
		return
	}
	
	// Generate temporary password
	tempPassword := h.authService.GenerateTemporaryPassword()
	if err := h.authService.SetTemporaryPassword(c.Request.Context(), device.DeviceID, tempPassword); err != nil {
		log.Printf("Failed to set temporary password: %v", err)
		// Continue anyway, device is registered
	}
	
	wsEndpoint := fmt.Sprintf("ws://%s:%d/signal/%s",
		h.config.Server.Host, h.config.Server.Port, device.DeviceID)
	
	c.JSON(http.StatusOK, gin.H{
		"device_id":         device.DeviceID,
		"device_uuid":       device.DeviceUUID,
		"temporary_password": tempPassword,
		"ws_endpoint":       wsEndpoint,
		"ttl_seconds":       int(service.TempPasswordTTL.Seconds()),
	})
	
	log.Printf("Device registered successfully: device_id=%s, temp_password=%s",
		device.DeviceID, tempPassword)
}

// GetDevice handles GET /api/v1/devices/:device_id
func (h *APIHandler) GetDevice(c *gin.Context) {
	deviceID := c.Param("device_id")
	
	device, err := h.deviceService.GetByDeviceID(c.Request.Context(), deviceID)
	if err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "device not found"})
		return
	}
	
	c.JSON(http.StatusOK, device)
}

// Health check endpoint
func (h *APIHandler) HealthCheck(c *gin.Context) {
	c.JSON(http.StatusOK, gin.H{
		"status": "ok",
		"service": "quickdesk-signaling",
	})
}
