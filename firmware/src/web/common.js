function confirmResetWifi(event) {
    const userConfirmed = confirm("Are you sure you want to reset the WiFi configuration?\n\nThe device will restart in Access Point mode and you will lose connection.");
    
    if (!userConfirmed) {
        event.preventDefault();
        return false;
    }
    
    return true;
}

function confirmReset(event) {
    const userConfirmed = confirm("Are you sure you want to reset the device configuration?\n\nThe device will restart in Access Point mode and you will lose connection and all saved parameters.");
    
    if (!userConfirmed) {
        event.preventDefault();
        return false;
    }
    
    return true;
}

function generateStatusSentence(data) {
    if (!data.esp_time) {
        return "Waiting for time synchronization (NTP)...";
    }
    
    const color = data.current_color;
    const brightness = data.brightness_percent;
    const totalLeds = data.num_leds || 12;
    const ledsOn = (data.current_leds_on === -1) ? totalLeds : data.current_leds_on;
    let validSchedules = (data.schedules || []).filter(s => s.time && s.color);
    let timeParts = data.esp_time.split(':');
    let displayTime = timeParts[0] + ':' + timeParts[1];
    
    if (validSchedules.length === 0) {
        return `It is currently <b>${displayTime}</b>. Led'o'clock is <b>${color}</b> (no schedules active).`;
    }
    
    let h = parseInt(timeParts[0]);
    let m = parseInt(timeParts[1]);
    let s = timeParts[2] ? parseInt(timeParts[2]) : 0;
    
    let currentDayIndex = (new Date().getDay() + 6) % 7;
    let nowSec = (currentDayIndex * 24 * 3600) + (h * 3600) + (m * 60) + s;
    
    let activeSched = null;
    let nextSched = null;
    let minElapsed = 604801;
    let minRemaining = 604801;
    
    for (let i = 0; i < validSchedules.length; i++) {
        let sched = validSchedules[i];
        let schedParts = sched.time.split(':');
        let schedH = parseInt(schedParts[0]);
        let schedM = parseInt(schedParts[1]);
        let daysArr = sched.days || [true, true, true, true, true, true, true];
        
        for (let day = 0; day < 7; day++) {
            if (daysArr[day]) {
                let schedSec = (day * 24 * 3600) + (schedH * 3600) + (schedM * 60);
                let elapsed = nowSec - schedSec;
                
                if (elapsed < 0) {
                    elapsed += 604800;
                }
                
                if (elapsed < minElapsed) {
                    minElapsed = elapsed;
                    activeSched = sched;
                }
                
                let remaining = schedSec - nowSec;
                
                if (remaining <= 0) {
                    remaining += 604800;
                }
                
                if (remaining < minRemaining) {
                    minRemaining = remaining;
                    nextSched = sched;
                }
            }
        }
    }
    
    if (!activeSched || !nextSched) {
        return `It is currently <b>${displayTime}</b>. Led'o'clock is <b>${color}</b> (no schedules active).`;
    }
    
    let total = minElapsed + minRemaining;
    let percent = total > 0 ? Math.round((minElapsed / total) * 100) : 0;
    let sentence = `It is currently <b>${displayTime}</b>. Led'o'clock is <b>${color}</b> at <b>${brightness}%</b> brightness since ${activeSched.time}. `;
    
    if (activeSched.countdown && color !== 'off') {
        sentence += `We are at <b>${percent}%</b> of the time passed up to ${nextSched.time} (when it will become <b>${nextSched.color}</b>). `;
        sentence += `So <b>${ledsOn}</b> out of ${totalLeds} LEDs ${ledsOn > 1 ? 'are' : 'is'} on.`;
    } else if (color !== 'off') {
        sentence += `It will stay lit up to ${nextSched.time} (when it will become <b>${nextSched.color}</b>).`;
    } else {
        sentence += `It will remain off up to ${nextSched.time} (when it will become <b>${nextSched.color}</b>).`;
    }
    
    return sentence;
}