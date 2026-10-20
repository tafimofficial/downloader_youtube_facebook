document.addEventListener('DOMContentLoaded', () => {
  const statusEl = document.getElementById('status');
  const statusText = document.getElementById('status-text');
  const btnTest = document.getElementById('btnTest');

  function checkConnection() {
    fetch('http://127.0.0.1:9898/ping')
      .then(res => res.json())
      .then(data => {
        statusEl.className = 'status-badge status-connected';
        statusText.textContent = 'Connected to Tafim';
      })
      .catch(() => {
        statusEl.className = 'status-badge status-disconnected';
        statusText.textContent = 'Tafim Disconnected';
      });
  }

  btnTest.addEventListener('click', checkConnection);
  checkConnection();
});
