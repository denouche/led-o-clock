async function fetchStatus() {
    try {
        const response = await fetch('/status');
        
        if (response.ok) {
            const data = await response.json();
            document.getElementById('status-container').style.display = 'block';
            document.getElementById('live-status-text').innerHTML = generateStatusSentence(data);
        }
    } catch (e) {
        console.error("Failed to fetch status");
    }
}

async function checkFirmwareVersion() {
    const checkingEl = document.getElementById('firmware-current-checking');
    const currentEl = document.getElementById('firmware-current');
    const currentVersionEl = document.getElementById('firmware-current-version');
    const updateNewEl = document.getElementById('firmware-update-new');
    const updateNewVersionEl = document.getElementById('firmware-update-new-version');
    const updateUptodateEl = document.getElementById('firmware-update-uptodate');

    checkingEl.style.display = '';
    currentEl.style.display = 'none';
    currentVersionEl.textContent = '';
    updateNewEl.style.display = 'none';
    updateNewVersionEl.textContent = '';
    updateUptodateEl.style.display = 'none';

    try {
        const response = await fetch('/firmware_check');
        checkingEl.style.display = 'none';
        if (response.ok) {
            const data = await response.json();
            currentEl.style.display = '';
            currentVersionEl.textContent = data.current_version || '?';
            if (data.update_available) {
                updateNewEl.style.display = '';
                updateUptodateEl.style.display = 'none';
                updateNewVersionEl.textContent = data.latest_version;
            } else if (data.latest_version) {
                updateNewEl.style.display = 'none';
                updateUptodateEl.style.display = '';
                updateNewVersionEl.textContent = '';
            } else {
                updateNewEl.style.display = 'none';
                updateUptodateEl.style.display = 'none';
                currentVersionEl.textContent += ' (unable to check for updates)';
            }
        } else {
            currentEl.style.display = '';
            currentVersionEl.textContent = 'Failed to check for updates.';
            updateNewEl.style.display = 'none';
            updateUptodateEl.style.display = 'none';
            updateNewVersionEl.textContent = '';
        }
    } catch (e) {
        checkingEl.style.display = 'none';
        currentEl.style.display = '';
        currentVersionEl.textContent = 'Unable to reach the device.';
        updateNewEl.style.display = 'none';
        updateUptodateEl.style.display = 'none';
        updateNewVersionEl.textContent = '';
    }
}


function escapeHtml(text) {
    if (!text) return '';
    return text.replace(/[&<>"']/g, function (c) {
        return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;','\'':'&#39;'}[c];
    });
}

async function triggerFirmwareUpdate() {
    const btn = document.getElementById('btn-firmware-update');
    btn.disabled = true;
    try {
        const response = await fetch('/firmware_update', { method: 'POST' });
        if (response.ok) {
            showMessage('Firmware update started. The device will restart when complete.');
        } else {
            showMessage('Firmware update request failed.', 'error');
        }
    } catch (e) {
        showMessage('Unable to reach the device.', 'error');
    } finally {
        btn.disabled = false;
    }
}

document.addEventListener('DOMContentLoaded', () => {
    fetchStatus();
    checkFirmwareVersion();
});