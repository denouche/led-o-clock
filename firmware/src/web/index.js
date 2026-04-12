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

async function triggerFirmwareUpdate() {
    const btn = document.getElementById('btn-firmware-update');
    btn.disabled = true;
    try {
        const response = await fetch('/firmware_update', { method: 'POST' });
        if (response.ok) {
            showMessage('Firmware update check started. If an update is available, it will install soon.');
        } else {
            showMessage('Firmware update request failed.', 'error');
        }
    } catch (e) {
        showMessage('Unable to reach the device.', 'error');
    } finally {
        btn.disabled = false;
    }
}

document.addEventListener('DOMContentLoaded', fetchStatus);