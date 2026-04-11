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

document.addEventListener('DOMContentLoaded', fetchStatus);