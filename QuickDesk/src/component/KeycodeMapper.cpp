// Copyright 2026 QuickDesk Authors

#include "KeycodeMapper.h"

#include <QGuiApplication>
#include <QKeyEvent>

namespace quickdesk {

// nativeModifiers bit positions for lock keys, as packed by Qt platform plugins.
// Windows  (QWindowsKeyMapper):     CapsLock=0x100  NumLock=0x200  ScrollLock=0x400
// macOS    (NSEvent.modifierFlags): CapsLock=0x10000 (no NumLock/ScrollLock)
#ifdef Q_OS_WIN
constexpr quint32 kNativeCapsLock   = 0x00000100;
constexpr quint32 kNativeNumLock    = 0x00000200;
constexpr quint32 kNativeScrollLock = 0x00000400;
#elif defined(Q_OS_MAC)
constexpr quint32 kNativeCapsLock   = 0x00010000;
constexpr quint32 kNativeNumLock    = 0;
constexpr quint32 kNativeScrollLock = 0;
#else
constexpr quint32 kNativeCapsLock   = 0;
constexpr quint32 kNativeNumLock    = 0;
constexpr quint32 kNativeScrollLock = 0;
#endif

KeyboardStateTracker::KeyboardStateTracker(QObject* parent)
    : QObject(parent)
{
    if (auto* app = QGuiApplication::instance()) {
        app->installEventFilter(this);
    }
}

bool KeyboardStateTracker::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress ||
        event->type() == QEvent::KeyRelease) {
        auto* ke = static_cast<QKeyEvent*>(event);
        m_cachedNativeModifiers = ke->nativeModifiers();
#ifdef Q_OS_MAC
        m_cachedNativeKeycode = ke->nativeVirtualKey();
#else
        m_cachedNativeKeycode = ke->nativeScanCode();
#endif
    }
    return QObject::eventFilter(obj, event);
}

int KeyboardStateTracker::getLastNativeKeycode() const
{
    return static_cast<int>(m_cachedNativeKeycode);
}

int KeyboardStateTracker::getLockStates() const
{
    int states = 0;
    if (kNativeCapsLock && (m_cachedNativeModifiers & kNativeCapsLock))
        states |= 1;
    if (kNativeNumLock && (m_cachedNativeModifiers & kNativeNumLock))
        states |= 2;
    if (kNativeScrollLock && (m_cachedNativeModifiers & kNativeScrollLock))
        states |= 4;
    return states;
}

} // namespace quickdesk
