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
    const infoEl = document.getElementById('firmware-info');
    const updateBtn = document.getElementById('btn-firmware-update');
    const checkBtn = document.getElementById('btn-firmware-check');
    
    infoEl.textContent = 'Checking for updates...';
    updateBtn.style.display = 'none';
    checkBtn.disabled = true;

    try {
        const response = await fetch('/firmware_check');
        if (response.ok) {
            const data = await response.json();
            if (data.update_available) {
                infoEl.innerHTML = 'current: <b>' + data.current_version + '</b>, new version available: <b>' + data.latest_version + '</b>';
                updateBtn.style.display = 'inline-block';
            } else if (data.latest_version) {
                infoEl.innerHTML = 'current: <b>' + data.current_version + '</b> (up to date)';
            } else {
                infoEl.innerHTML = 'current: <b>' + data.current_version + '</b> (unable to check for updates)';
            }
        } else {
            infoEl.textContent = 'Failed to check for updates.';
        }
    } catch (e) {
        infoEl.textContent = 'Unable to reach the device.';
    } finally {
        checkBtn.disabled = false;
    }
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