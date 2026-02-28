// Copyright 2026 QuickDesk Authors

#pragma once

#include <QObject>

#include "base/singleton.h"

namespace quickdesk {

/**
 * @brief Tracks keyboard lock key states via QKeyEvent::nativeModifiers().
 *
 * Installs an event filter on QGuiApplication to cache nativeModifiers
 * from every key event. The lock key bits are platform-plugin–specific
 * constants set by QWindowsKeyMapper / QCocoaKeyMapper.
 */
class KeyboardStateTracker : public QObject,
                             public base::Singleton<KeyboardStateTracker> {
    Q_OBJECT

public:
    KeyboardStateTracker(QObject* parent = nullptr);

    /// @return Bitmask: 1=CapsLock, 2=NumLock, 4=ScrollLock
    Q_INVOKABLE int getLockStates() const;

    /// @return Platform-native keycode suitable for Chromium's
    ///         KeycodeConverter::NativeKeycodeToUsbKeycode():
    ///         - Windows: nativeScanCode (hardware scan code)
    ///         - macOS:   nativeVirtualKey (macOS virtual key code, [NSEvent keyCode])
    Q_INVOKABLE int getLastNativeKeycode() const;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    quint32 m_cachedNativeModifiers = 0;
    quint32 m_cachedNativeKeycode = 0;
};

} // namespace quickdesk
