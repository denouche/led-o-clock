#ifndef WIFI_MANAGER_HTML_H
#define WIFI_MANAGER_HTML_H

/**
 * Custom header for the WiFiManager portal
 */
static const char WIFI_MANAGER_CUSTOM_HEAD[] PROGMEM = R"rawliteral(
<style>
    .success-msg { 
        font-family: sans-serif; 
        text-align: center; 
        padding: 20px; 
        background: #ffffff; 
        border-radius: 8px; 
        box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        margin: 20px auto; 
        max-width: 400px; 
        color: #333;
    }
    .loader {
        border: 4px solid #f3f3f3; 
        border-top: 4px solid #4CAF50;
        border-radius: 50%; 
        width: 30px; 
        height: 30px;
        animation: spin 2s linear infinite; 
        margin: 20px auto;
    }
    #countdown-text { 
        font-weight: bold; 
        margin: 15px 0; 
    }
    #countdown { 
        color: #dc3545; 
    }
    .url-container {
        background: #f8f9fa; 
        border: 1px solid #ddd; 
        border-radius: 5px;
        padding: 15px; 
        margin: 15px 0; 
        display: flex; 
        flex-direction: column; 
        align-items: center;
    }
    #device-url { 
        font-weight: bold; 
        color: #4CAF50; 
        font-size: 1.2em; 
        word-break: break-all; 
        margin-bottom: 10px; 
        text-align: center; 
    }
    .copy-btn {
        background: #4CAF50; 
        color: white; 
        border: none; 
        border-radius: 4px;
        padding: 8px 12px; 
        cursor: pointer; 
        font-size: 0.9em; 
        white-space: nowrap;
    }
    .copy-btn:active { 
        background: #45a049; 
    }
    @keyframes spin { 
        0% { transform: rotate(0deg); } 
        100% { transform: rotate(360deg); } 
    }
</style>
<script>
    /**
     * Copy the device local URL to clipboard
     */
    function copyToClipboard(btn) {
        const text = document.getElementById('device-url').innerText;
        const originalText = btn.innerText;
        
        const success = () => {
            btn.innerText = 'Copied!';
            btn.style.background = '#28a745';
            btn.disabled = true;
            setTimeout(() => { 
                btn.innerText = originalText; 
                btn.style.background = '#4CAF50';
                btn.disabled = false;
            }, 2000);
        };

        if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(text).then(success).catch(() => {
                fallbackCopy(text, success);
            });
        } else {
            fallbackCopy(text, success);
        }
    }

    /**
     * Fallback copy method for older browsers
     */
    function fallbackCopy(text, callback) {
        var textArea = document.createElement("textarea");
        textArea.value = text;
        textArea.style.position = "fixed";
        document.body.appendChild(textArea);
        textArea.focus();
        textArea.select();
        
        try { 
            if (document.execCommand('copy')) {
                callback(); 
            }
        } catch (err) {
            // Ignore error
        }
        
        document.body.removeChild(textArea);
    }

    /**
     * Display a countdown after saving WiFi credentials
     */
    if (window.location.href.indexOf('save') > -1) {
        document.addEventListener('DOMContentLoaded', function() {
            document.body.style.backgroundColor = "#f0f2f5";
            document.body.innerHTML = `
                <div class="success-msg">
                    <div class="loader"></div>
                    <h1>Configuration Saved!</h1>
                    <p>Led'o'clock is connecting to your WiFi.</p>
                    <hr style="border:0; border-top:1px solid #eee;">
                    <p id="countdown-text">This window will close in <span id="countdown">30</span> seconds.</p>
                    <div class="url-container">
                        After reboot, access the device at: <br>
                        <span id="device-url">http://%HOSTNAME%.local</span>
                        <button class="copy-btn" onclick="copyToClipboard(this)">Copy to clipboard</button>
                    </div>
                </div>
            `;
            
            var seconds = 30;
            
            var countdown = setInterval(function() {
                seconds--;
                var countdownSpan = document.getElementById('countdown');
                
                if (countdownSpan) {
                    countdownSpan.textContent = seconds;
                }
                
                if (seconds <= 0) {
                    clearInterval(countdown);
                }
            }, 1000);
        });
    }
</script>
)rawliteral";

#endif