// ============================================================
//  MONITORING SPORT HALL SCU — script.js
//  Sensor 1 : Titik Ukur      (d97fa97... HiveMQ)
//  Sensor 2 : Titik Referensi (022f2c5... HiveMQ)
// ============================================================

// ── MQTT Config ───────────────────────────────────────────────
const MQTT_CONFIGS = {
    s1: {
        hostname: 'd97fa97042434738a712d06b663db901.s1.eu.hivemq.cloud',
        port: 8884,
        username: 'hivemq.webclient.1769348223798',
        password: 'E0MR9o&!I7dP1zct#:bD',
        clientId: 'web-s1-' + Math.random().toString(16).substr(2, 8),
        protocol: 'wss',
        clean: true,
        reconnectPeriod: 3000,
        connectTimeout: 5000
    },
    s2: {
        hostname: '022f2c5e9cbd472497f3ba11a75c43fa.s1.eu.hivemq.cloud',
        port: 8884,
        username: 'hivemq.webclient.1779087469304',
        password: '9pVH<0T12uyzwK,Ra#*B',
        clientId: 'web-s2-' + Math.random().toString(16).substr(2, 8),
        protocol: 'wss',
        clean: true,
        reconnectPeriod: 3000,
        connectTimeout: 5000
    }
};

const TOPIC = 'building-tilt/sensor-data';

// ── Helpers ───────────────────────────────────────────────────
function getAngleColor(angle) {
    const a = Math.abs(angle);
    if (a < 5.0)  return '#28a745';
    if (a < 10.0) return '#ffc107';
    if (a < 20.0) return '#ff9800';
    return '#dc3545';
}

function getBuildingStatus(maxAngle) {
    const a = Math.abs(maxAngle);
    if (a < 5.0)  return 'AMAN';
    if (a < 10.0) return 'WASPADA';
    if (a < 20.0) return 'SIAGA';
    return 'BAHAYA';
}

// ── Update sensor display ─────────────────────────────────────
function updateSensorUI(id, data) {
    const rollEl   = document.getElementById(`${id}-roll`);
    const pitchEl  = document.getElementById(`${id}-pitch`);
    const statusEl = document.getElementById(`${id}-status`);

    if (rollEl) {
        rollEl.textContent = data.roll.toFixed(2) + '°';
        rollEl.style.color = getAngleColor(data.roll);
    }
    if (pitchEl) {
        pitchEl.textContent = data.pitch.toFixed(2) + '°';
        pitchEl.style.color = getAngleColor(data.pitch);
    }
    if (statusEl) {
        const max    = Math.max(Math.abs(data.roll), Math.abs(data.pitch));
        const status = getBuildingStatus(max);
        statusEl.textContent = status;
        statusEl.className   = `data-value status-${status.toLowerCase()}`;
    }

    // Alert banner hanya untuk Sensor 1 (titik ukur)
    if (id === 's1') updateAlertBanner(data);
}

// ── Alert Banner (berdasarkan Sensor 1) ──────────────────────
function updateAlertBanner(data) {
    const banner = document.getElementById('alert-banner');
    const title  = document.getElementById('alert-title');
    const msg    = document.getElementById('alert-message');
    if (!banner) return;

    const maxAngle = Math.max(Math.abs(data.roll), Math.abs(data.pitch));

    if (maxAngle >= 20.0) {
        banner.style.display = 'flex';
        banner.className = 'alert-banner alert-bahaya';
        title.textContent = 'BAHAYA: Kemiringan Ekstrem!';
        msg.textContent   = `Sudut kemiringan mencapai ${maxAngle.toFixed(2)}°. Segera evakuasi dan hubungi petugas!`;
    } else if (maxAngle >= 10.0) {
        banner.style.display = 'flex';
        banner.className = 'alert-banner alert-siaga';
        title.textContent = 'SIAGA: Kemiringan Tinggi';
        msg.textContent   = `Sudut kemiringan mencapai ${maxAngle.toFixed(2)}°. Lakukan pemeriksaan segera.`;
    } else if (maxAngle >= 5.0) {
        banner.style.display = 'flex';
        banner.className = 'alert-banner alert-waspada';
        title.textContent = 'WASPADA: Kemiringan Sedang';
        msg.textContent   = `Sudut kemiringan menunjukkan ${maxAngle.toFixed(2)}°. Pantau terus kondisi.`;
    } else {
        banner.style.display = 'none';
    }
}

// ── Connection Status UI ──────────────────────────────────────
function updateConnStatus(id, status) {
    const dot  = document.getElementById(`${id}-dot`);
    const text = document.getElementById(`${id}-conn-text`);
    if (!dot || !text) return;

    dot.className = 'status-dot';
    const MAP = {
        connected:    ['connected',    'Terhubung ke HiveMQ Cloud'],
        disconnected: ['disconnected', 'Terputus dari HiveMQ'],
        connecting:   ['connecting',   'Menghubungkan ke HiveMQ...'],
        error:        ['disconnected', 'Error koneksi HiveMQ']
    };
    const [cls, label] = MAP[status] || ['disconnected', 'Tidak Diketahui'];
    dot.classList.add(cls);
    text.textContent = label;
}

// ── Init MQTT ─────────────────────────────────────────────────
function initMQTT(id) {
    const cfg = MQTT_CONFIGS[id];
    const url = `${cfg.protocol}://${cfg.hostname}:${cfg.port}/mqtt`;

    console.log(`[${id.toUpperCase()}] Connecting → ${cfg.hostname}`);
    updateConnStatus(id, 'connecting');

    const client = mqtt.connect(url, {
        username:        cfg.username,
        password:        cfg.password,
        clientId:        cfg.clientId,
        clean:           cfg.clean,
        reconnectPeriod: cfg.reconnectPeriod,
        connectTimeout:  cfg.connectTimeout
    });

    client.on('connect', () => {
        console.log(`[${id.toUpperCase()}] Connected ✓`);
        updateConnStatus(id, 'connected');

        client.subscribe(TOPIC, { qos: 0 }, (err) => {
            if (err) console.error(`[${id.toUpperCase()}] Subscribe error:`, err);
            else     console.log(`[${id.toUpperCase()}] Subscribed: ${TOPIC}`);
        });
    });

    client.on('message', (topic, message) => {
        if (topic !== TOPIC) return;
        try {
            const data = JSON.parse(message.toString());
            if (typeof data.roll !== 'number' || typeof data.pitch !== 'number') return;
            updateSensorUI(id, data);
        } catch (err) {
            console.error(`[${id.toUpperCase()}] Parse error:`, err.message);
        }
    });

    client.on('error',     () => updateConnStatus(id, 'error'));
    client.on('close',     () => updateConnStatus(id, 'disconnected'));
    client.on('offline',   () => updateConnStatus(id, 'disconnected'));
    client.on('reconnect', () => updateConnStatus(id, 'connecting'));
}

// ── Date / Time ───────────────────────────────────────────────
function updateDateTime() {
    const now  = new Date();
    const opts = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' };
    const dateEl = document.getElementById('current-date');
    const timeEl = document.getElementById('current-time');
    const yearEl = document.getElementById('current-year');
    if (dateEl) dateEl.textContent = now.toLocaleDateString('id-ID', opts);
    if (timeEl) timeEl.textContent = now.toLocaleTimeString('id-ID');
    if (yearEl) yearEl.textContent = now.getFullYear();
}

// ── Bootstrap ─────────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', () => {
    console.log('=== Sport Hall Dual-Sensor Monitoring ===');
    updateDateTime();
    setInterval(updateDateTime, 1000);
    initMQTT('s1');
    initMQTT('s2');
});
