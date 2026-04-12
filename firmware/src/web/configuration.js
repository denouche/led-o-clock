const MAX_SCHEDULES = 12;
let isColorModified = false;
let isBrightnessDirty = false;
let isColorsDirty = false;
let isTimezoneDirty = false;
let isSchedulesDirty = false;
let dynamicColors = [];

function showSaveButton() {
    document.getElementById('mainSaveButton').style.display = 'block';
}

function markColorModified() {
    isColorModified = true;
    showSaveButton();
}

function markBrightnessDirty() {
    isBrightnessDirty = true;
    showSaveButton();
}

function markTimezoneDirty() {
    isTimezoneDirty = true;
    showSaveButton();
}

function markColorsDirty() {
    isColorsDirty = true;
    showSaveButton();
}

function markSchedulesDirty() {
    isSchedulesDirty = true;
    showSaveButton();
}

function updateAllColorDropdowns() {
    const selects = document.querySelectorAll('.color-dropdown');
    
    selects.forEach(select => {
        const previousValue = select.value;
        select.innerHTML = '<option value="off">Off</option>';
        
        dynamicColors.forEach(c => {
            const opt = document.createElement('option');
            opt.value = c.name;
            opt.textContent = c.name.charAt(0).toUpperCase() + c.name.slice(1);
            select.appendChild(opt);
        });
        
        if (previousValue) {
            select.value = previousValue;
        }
    });
}

function renderColorEditor() {
    const container = document.getElementById('colors-list');
    container.innerHTML = '';
    
    dynamicColors.forEach((c, index) => {
        const div = document.createElement('div');
        div.className = 'schedule-controls';
        div.style.marginBottom = '10px';
        
        div.innerHTML = `
            <input type="text" id="color_name_${index}" value="${c.name}" oninput="syncDynamicColors()" style="width: 100px;">
            <input type="color" id="color_hex_${index}" value="${c.hex}" oninput="syncDynamicColors()">
            <button type="button" class="clear-btn" style="margin-left:auto" onclick="removeColorRow(${index})">X</button>
        `;
        
        container.appendChild(div);
    });
    
    updateAllColorDropdowns();
}

function syncDynamicColors() {
    dynamicColors = [];
    let i = 0;
    let currentNameInput = document.getElementById(`color_name_${i}`);
    
    while (currentNameInput) {
        const name = currentNameInput.value.trim().toLowerCase();
        const hex = document.getElementById(`color_hex_${i}`).value;
        
        if (name) {
            dynamicColors.push({ name: name, hex: hex });
        }
        
        i++;
        currentNameInput = document.getElementById(`color_name_${i}`);
    }
    
    updateAllColorDropdowns();
    markColorsDirty();
}

function addColorRow() {
    dynamicColors.push({ name: "new", hex: "#ffffff" });
    renderColorEditor();
    markColorsDirty();
}

function removeColorRow(index) {
    dynamicColors.splice(index, 1);
    renderColorEditor();
    markColorsDirty();
}

function showMessage(text, type = 'success') {
    let toast = document.getElementById('toast');
    if (!toast) {
        toast = document.createElement('div');
        toast.id = 'toast';
        document.body.appendChild(toast);
    }

    // Reset any pending hide timer
    clearTimeout(toast._hideTimer);

    toast.textContent = text;
    toast.className = `toast ${type}`;

    // Trigger reflow to restart the CSS transition if called twice quickly
    void toast.offsetWidth;
    toast.classList.add('show');

    toast._hideTimer = setTimeout(() => {
        toast.classList.remove('show');
    }, 3000);
}

function checkScheduleLimit() {
    const currentCount = document.querySelectorAll('.schedule').length;
    const addBtn = document.getElementById('addScheduleBtn');
    
    if (currentCount >= MAX_SCHEDULES) {
        addBtn.style.display = 'none';
    } else {
        addBtn.style.display = 'block';
    }
}

function updateScheduleLabels() {
    const blocks = document.querySelectorAll('.schedule');
    
    blocks.forEach((block, index) => {
        const label = block.querySelector('.schedule-label');
        if (label) {
            label.innerText = `Schedule #${index + 1}`;
        }
    });
}

function addScheduleRow(timeValue, colorValue, isCountdown, daysArray, brightnessValue) {
    const container = document.getElementById('schedules');
    const currentCount = container.querySelectorAll('.schedule').length;
    
    if (currentCount >= MAX_SCHEDULES) {
        return;
    }

    const days = daysArray || [true, true, true, true, true, true, true];
    const dayNames = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'];
    const brightnessPercent = Number.isFinite(brightnessValue)
        ? Math.min(100, Math.max(0, brightnessValue))
        : 50;

    const div = document.createElement('div');
    div.className = 'schedule';
    
    const checkedAttribute = isCountdown ? 'checked' : '';
    
    let htmlContent = `
        <div class="schedule-label">Schedule #${currentCount + 1}</div>
        <div class="schedule-controls">
            <input type="time" class="sched-time" value="${timeValue}" oninput="markSchedulesDirty()">
            <select class="color-dropdown sched-color" onchange="markSchedulesDirty()">
            </select>
            <label>
                <input type="checkbox" class="sched-countdown" ${checkedAttribute} onchange="markSchedulesDirty()"> Countdown
            </label>
            <button type="button" class="clear-btn" style="margin-left:auto" onclick="removeScheduleRow(this)">X</button>
        </div>
        <div class="schedule-options">
            <span>Brightness:</span>
            <input type="range" class="sched-brightness" min="0" max="100" value="${brightnessPercent}" oninput="this.nextElementSibling.textContent=this.value; markSchedulesDirty()" style="flex-grow: 1;">
            <span class="sched-brightness-value">${brightnessPercent}</span>%
        </div>
        <div class="schedule-days">
            <span>Applies on:</span>
    `;

    for (let j = 0; j < 7; j++) {
        const dayChecked = days[j] ? 'checked' : '';
        htmlContent += `
            <label>
                <input type="checkbox" class="day-cb" value="${j}" ${dayChecked} onchange="markSchedulesDirty()"> ${dayNames[j]}
            </label>
        `;
    }

    htmlContent += `</div>`;
    div.innerHTML = htmlContent;
    container.appendChild(div);
    updateAllColorDropdowns();
    
    if (colorValue) {
        const selectElement = div.querySelector('.sched-color');
        selectElement.value = colorValue;
    }
    
    checkScheduleLimit();
    updateScheduleLabels();
    
    if (timeValue === '') {
        markSchedulesDirty();
    }
}

function removeScheduleRow(buttonElement) {
    const row = buttonElement.closest('.schedule');
    row.remove();
    checkScheduleLimit();
    updateScheduleLabels();
    markSchedulesDirty();
}

async function loadConfig() {
    try {
        const res = await fetch('/status');
        
        if (!res.ok) {
            document.querySelector('.spinner').style.display = 'none';
            document.getElementById('loader-text').innerHTML = 'Error loading configuration. <br>Please refresh the page.';
            return;
        }
        
        const d = await res.json();
        
        document.getElementById('loader').style.display = 'none';
        document.getElementById('config-content').style.display = 'block';
        document.getElementById('status-container').style.display = 'block';
        document.getElementById('live-status-text').innerHTML = generateStatusSentence(d);
        
        if (d.colors) {
            dynamicColors = d.colors;
            renderColorEditor();
        }

        document.getElementById('current-color').value = d.current_color;
        document.getElementById('brightness').value = d.brightness_percent;
        document.getElementById('brightness-value').textContent = d.brightness_percent;
        
        if (d.timezone) {
            document.getElementById('timezone').value = d.timezone;
        }
        
        document.getElementById('schedules').innerHTML = '';
        
        d.schedules.forEach((s) => {
            addScheduleRow(s.time, s.color, s.countdown, s.days, s.brightness);
        });
        
        isColorModified = false; 
        isBrightnessDirty = false;
        isColorsDirty = false;
        isTimezoneDirty = false;
        isSchedulesDirty = false;
        document.getElementById('mainSaveButton').style.display = 'none'; 
        
    } catch (err) { 
        console.error("Load failed", err); 
        document.querySelector('.spinner').style.display = 'none';
        document.getElementById('loader-text').innerHTML = 'Connection error. <br>Please check your WiFi and refresh.';
    }
}

document.getElementById('scheduleForm').onsubmit = async (e) => {
    e.preventDefault();
    const btn = document.getElementById('mainSaveButton');
    btn.disabled = true;
    btn.textContent = "SAVING...";

    try {
        if (isTimezoneDirty) {
            const tz = encodeURIComponent(document.getElementById('timezone').value);
            await fetch(`/set_timezone?value=${tz}`);
        }

        if (isColorsDirty) {
            await fetch('/colors', {
                method: 'POST', 
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(dynamicColors)
            });
        }

        if (isSchedulesDirty) {
            const schedData = [];
            const scheduleBlocks = document.querySelectorAll('.schedule');
            
            scheduleBlocks.forEach((block) => {
                const t = block.querySelector('.sched-time').value;
                const c = block.querySelector('.sched-color').value;
                const cd = block.querySelector('.sched-countdown').checked;
                const dCbs = block.querySelectorAll('.day-cb');
                const daysArr = Array.from(dCbs).map(cb => cb.checked);
                const brightVal = parseInt(block.querySelector('.sched-brightness').value, 10);
                
                if (t && c) {
                    schedData.push({ time: t, color: c, countdown: cd, brightness: brightVal, days: daysArr });
                }
            });
            
            await fetch('/schedule', {
                method: 'POST', 
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(schedData)
            });
        }

        if (isColorModified) {
            const colorOverride = document.getElementById('current-color').value;
            await fetch(`/set_color?value=${colorOverride}`);
        }

        if (isBrightnessDirty) {
            const bright = document.getElementById('brightness').value;
            await fetch(`/set_brightness?value=${bright}`);
        }

        btn.disabled = false;
        btn.textContent = "SAVE CHANGES";
        btn.style.display = 'none'; 
        
        showMessage('Configuration saved successfully!');

        document.getElementById('config-content').style.display = 'none';
        document.getElementById('loader').style.display = 'block';
        
        loadConfig();
        
    } catch (err) {
        console.error("Save failed", err);
        btn.disabled = false;
        btn.textContent = "SAVE CHANGES";
        showMessage('Failed to save configuration. Please try again.', 'error');
    }
};

document.getElementById('brightness').oninput = (e) => {
    document.getElementById('brightness-value').textContent = e.target.value;
    markBrightnessDirty();
};

loadConfig();