/**
 * UserSync — real-time sync of user data (devices, favorites) over WebSocket.
 */

const DEVICE_EVENTS = new Set([
  'device_online',
  'device_offline',
  'device_access_code_changed',
  'device_remark_changed',
]);

const FAVORITE_EVENTS = new Set([
  'favorite_added',
  'favorite_updated',
  'favorite_removed',
]);

class UserSync extends EventTarget {
  constructor() {
    super();
    /** @type {WebSocket|null} */
    this._ws = null;
    this._reconnectTimer = null;
    this._reconnectDelay = 5000;
    this._stopped = true;
    this._wsBaseUrl = '';
    this._token = '';
  }

  /**
   * Open a WebSocket connection for user-data sync.
   * @param {string} wsBaseUrl  Signaling server URL (ws:// or wss://).
   * @param {string} token      Auth token passed as a query parameter.
   */
  connect(wsBaseUrl, token) {
    this._stopped = false;
    this._wsBaseUrl = wsBaseUrl;
    this._token = token;
    this._open();
  }

  /** Cleanly close the WebSocket and stop reconnecting. */
  disconnect() {
    this._stopped = true;
    clearTimeout(this._reconnectTimer);
    this._reconnectTimer = null;
    if (this._ws) {
      this._ws.onclose = null;
      this._ws.close();
      this._ws = null;
    }
  }

  // --- internal ------------------------------------------------------------

  _open() {
    const base = this._wsBaseUrl.replace(/\/+$/, '');
    const url = `${base}/api/v1/user/sync?token=${encodeURIComponent(this._token)}`;

    this._ws = new WebSocket(url);

    this._ws.onopen = () => {
      this.dispatchEvent(new CustomEvent('connected'));
    };

    this._ws.onmessage = (event) => this._onMessage(event);

    this._ws.onclose = () => {
      this.dispatchEvent(new CustomEvent('disconnected'));
      this._scheduleReconnect();
    };

    this._ws.onerror = () => {
      // The browser will fire onclose right after onerror, so reconnect is
      // handled there. Nothing extra needed here.
    };
  }

  _scheduleReconnect() {
    if (this._stopped) return;
    clearTimeout(this._reconnectTimer);
    this._reconnectTimer = setTimeout(() => this._open(), this._reconnectDelay);
  }

  /**
   * Parse an incoming JSON message and dispatch the appropriate custom event.
   * @param {MessageEvent} event
   */
  _onMessage(event) {
    let msg;
    try {
      msg = JSON.parse(event.data);
    } catch {
      return;
    }

    const { type } = msg;
    if (!type) return;

    if (DEVICE_EVENTS.has(type)) {
      this.dispatchEvent(new CustomEvent('devices-changed', { detail: msg }));
    } else if (FAVORITE_EVENTS.has(type)) {
      this.dispatchEvent(new CustomEvent('favorites-changed', { detail: msg }));
    }
  }
}

export const userSync = new UserSync();
