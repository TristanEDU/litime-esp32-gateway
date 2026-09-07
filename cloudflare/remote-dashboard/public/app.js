(() => {
	const $ = (id) => document.getElementById(id);
	const connection = $('connection');
	const receivedAt = $('received-at');
	const offlinePanel = $('offline-panel');
	const offlineTitle = $('offline-title');
	const offlineCopy = $('offline-copy');
	const cells = $('cells');
	let socket;
	let reconnectTimer;
	let statusPollTimer;
	let reconnectAttempts = 0;
	let receivedStatusAt = 0;

	const value = (number, decimals = 1) => (Number.isFinite(Number(number)) ? Number(number).toFixed(decimals) : '—');
	const finite = (number) => Number.isFinite(Number(number));

	function setConnection(text, state) {
		connection.textContent = text;
		connection.className = `badge ${state}`;
	}
	function setOffline(title, copy, visible = true) {
		offlineTitle.textContent = title;
		offlineCopy.textContent = copy;
		offlinePanel.hidden = !visible;
	}
	function setText(id, text) {
		$(id).textContent = text;
	}

	function showBattery(battery) {
		const isConnected = battery.connected === true;
		const isValid = battery.valid === true;
		const hasLiveReading = isConnected && isValid;
		const soc = finite(battery.soc) ? Math.max(0, Math.min(100, Number(battery.soc))) : 0;
		setText('soc', hasLiveReading ? Math.round(soc) : '—');
		$('soc-bar').style.width = `${hasLiveReading ? soc : 0}%`;
		setText('voltage', hasLiveReading ? value(battery.voltage, 2) : '—');
		setText('current', hasLiveReading ? value(battery.current, 2) : '—');
		setText('power', hasLiveReading ? value(battery.power, 0) : '—');
		setText('capacity', hasLiveReading ? `${value(battery.remainingAh)} / ${value(battery.capacityAh)} Ah` : '—');
		setText('cell-count', hasLiveReading ? (finite(battery.cellCount) ? `${battery.cellCount} cells` : '—') : '—');
		setText('cell-delta', hasLiveReading ? `${value(battery.cellDeltaMv)} mV` : '—');
		setText('min-cell', hasLiveReading ? `${value(battery.minCell, 3)} V` : '—');
		setText('max-cell', hasLiveReading ? `${value(battery.maxCell, 3)} V` : '—');
		setText('cell-temp', hasLiveReading ? `${value(battery.cellTemp)} °C` : '—');
		setText('mosfet-temp', hasLiveReading ? `${value(battery.mosfetTemp)} °C` : '—');
		const reportedCells = Array.isArray(battery.cells) ? battery.cells : [];
		cells.replaceChildren(
			...reportedCells.map((cell, index) => {
				const item = document.createElement('span');
				item.className = 'cell';
				item.textContent = `Cell ${index + 1}: ${value(cell, 3)} V`;
				return item;
			}),
		);
		setText('cell-summary', reportedCells.length ? `${reportedCells.length} reported` : 'No cells reported');
		if (hasLiveReading) {
			setText('validity', 'Live reading');
			$('validity').className = 'data-state good';
			setConnection('Live BLE telemetry', 'good');
			setOffline('', '', false);
		} else if (isConnected) {
			setText('validity', 'Waiting for telemetry');
			$('validity').className = 'data-state waiting';
			setConnection('Waiting for telemetry', 'waiting');
			setOffline(
				'Battery is connected, but no valid reading is available yet.',
				'The gateway remains online and will update this view after its next valid BLE status.',
				true,
			);
		} else {
			setText('validity', 'Battery disconnected');
			$('validity').className = 'data-state offline';
			setConnection('Battery disconnected', 'offline');
			setOffline(
				'The gateway cannot currently reach the LiTime battery.',
				'It will keep scanning and this page will update when a valid battery status returns.',
				true,
			);
		}
		receivedStatusAt = Date.now();
		refreshReceivedAt();
	}

	function refreshReceivedAt() {
		if (!receivedStatusAt) return;
		const seconds = Math.max(0, Math.round((Date.now() - receivedStatusAt) / 1000));
		receivedAt.textContent = seconds < 2 ? 'Status received just now' : `Status received ${seconds}s ago`;
	}
	function scheduleReconnect() {
		clearTimeout(reconnectTimer);
		clearInterval(statusPollTimer);
		const delay = Math.min(30000, 1000 * 2 ** Math.min(reconnectAttempts, 5));
		reconnectAttempts += 1;
		setConnection('Relay reconnecting…', 'connecting');
		setOffline('Remote relay connection interrupted.', `Trying again in ${Math.ceil(delay / 1000)} seconds.`, true);
		reconnectTimer = setTimeout(connect, delay);
	}
	function connect() {
		clearTimeout(reconnectTimer);
		if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) return;
		setConnection('Connecting to gateway…', 'connecting');
		setOffline('Connecting to the relay', 'The dashboard will request live status as soon as the secure connection opens.', true);
		const scheme = location.protocol === 'https:' ? 'wss:' : 'ws:';
		socket = new WebSocket(`${scheme}//${location.host}/browser`);
		socket.addEventListener('open', () => {
			reconnectAttempts = 0;
			setConnection('Requesting live status…', 'connecting');
			requestStatus();
			statusPollTimer = setInterval(requestStatus, 3000);
		});
		socket.addEventListener('message', (event) => {
			let message;
			try {
				message = JSON.parse(event.data);
			} catch {
				return;
			}
			if (message && message.type === 'status' && message.battery && typeof message.battery === 'object') showBattery(message.battery);
		});
		socket.addEventListener('error', () => socket.close());
		socket.addEventListener('close', scheduleReconnect);
	}

	function requestStatus() {
		if (socket?.readyState === WebSocket.OPEN) {
			socket.send(JSON.stringify({ type: 'get_status' }));
		}
	}
	window.addEventListener('online', connect);
	window.addEventListener('offline', () => {
		if (socket) socket.close();
	});
	setInterval(refreshReceivedAt, 1000);
	connect();
})();
