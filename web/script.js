const API_KEY = 'nexusgate-dev-key';
const METRICS_URL = 'http://localhost:8081/metrics';

let chart = null;
try {
    const ctx = document.getElementById('latencyChart').getContext('2d');
    chart = new Chart(ctx, {
    type: 'line',
    data: { labels: [], datasets: [{ label: 'Latência média (ms)', data: [], borderColor: '#5dade2', tension: 0.3 }] },
    options: { animation: false, scales: { y: { beginAtZero: true } } }
    });
} catch (e) {
    logDebug('Chart.js nao carregou, seguindo sem grafico: ' + e.message);
}

function logDebug(msg) {
    const el = document.getElementById('debugLog');
    const time = new Date().toLocaleTimeString();
    el.textContent = '[' + time + '] ' + msg + '\n' + el.textContent;
}

async function refresh() {
    try {
    const res = await fetch(METRICS_URL, {
        cache: 'no-store',
        headers: { 'X-API-Key': API_KEY }
    });

    if (!res.ok) {
        logDebug('Resposta HTTP nao-OK: ' + res.status);
        return;
    }

    const data = await res.json();

    document.getElementById('totalReq').textContent = data.totalRequests;
    document.getElementById('totalErr').textContent = data.totalErrors;
    document.getElementById('activeConn').textContent = data.activeConnections;
    document.getElementById('avgLat').textContent = data.avgLatencyMs.toFixed(2);

    if (chart) {
        const now = new Date().toLocaleTimeString();
        chart.data.labels.push(now);
        chart.data.datasets[0].data.push(data.avgLatencyMs);
        if (chart.data.labels.length > 20) {
            chart.data.labels.shift();
            chart.data.datasets[0].data.shift();
        }
        chart.update();
    }

    const rows = data.backends.map(b => `
        <tr>
        <td>${b.host}:${b.port}</td>
        <td><span class="dot ${b.healthy ? 'healthy' : 'unhealthy'}"></span>${b.healthy ? 'Saudável' : 'Indisponível'}</td>
        <td>${b.requests}</td>
        </tr>`).join('');
    document.getElementById('backendRows').innerHTML = rows;

    } catch (e) {
    logDebug('Falha no fetch: ' + e.message);
    }
}

refresh();
setInterval(refresh, 2000);