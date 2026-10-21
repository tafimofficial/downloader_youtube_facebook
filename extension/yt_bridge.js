// Tafim YouTube Main World Bridge
// Runs in YouTube page context so ytInitialPlayerResponse / player APIs are visible.

(function () {
  'use strict';

  function formatHeight(fmt) {
    if (fmt.height) return fmt.height;
    const label = String(fmt.qualityLabel || '');
    const m = label.match(/(\d{3,4})p/i);
    return m ? parseInt(m[1], 10) : 0;
  }

  function extractAndSendFormats() {
    try {
      const player = document.getElementById('movie_player') || document.querySelector('.html5-video-player');
      let formats = [];
      let playerResp = null;

      if (player && typeof player.getPlayerResponse === 'function') {
        playerResp = player.getPlayerResponse();
      }
      if (!playerResp && window.ytInitialPlayerResponse) {
        playerResp = window.ytInitialPlayerResponse;
      }
      if (!playerResp) {
        const watchFlexy = document.querySelector('ytd-watch-flexy');
        if (watchFlexy && watchFlexy.playerData) {
          playerResp = watchFlexy.playerData;
        }
      }

      if (playerResp && playerResp.streamingData) {
        const sd = playerResp.streamingData;
        const all = [...(sd.formats || []), ...(sd.adaptiveFormats || [])];
        for (const f of all) {
          if (!f || !f.itag) continue;
          const mime = f.mimeType || '';
          formats.push({
            itag: f.itag,
            qualityLabel: f.qualityLabel || '',
            quality: f.quality || '',
            mimeType: mime,
            contentLength: f.contentLength || '',
            bitrate: f.bitrate || 0,
            approxDurationMs: f.approxDurationMs || 0,
            width: f.width || 0,
            height: formatHeight(f),
            fps: f.fps || 0,
            url: f.url || '',
            isAudio: mime.toLowerCase().startsWith('audio/')
          });
        }
      }

      let qualityLevels = [];
      if (player && typeof player.getAvailableQualityLevels === 'function') {
        qualityLevels = player.getAvailableQualityLevels() || [];
      }

      window.postMessage({
        type: 'TAFIM_YT_PAGE_FORMATS',
        formats: formats,
        qualityLevels: qualityLevels
      }, '*');
    } catch (_) {}
  }

  window.addEventListener('message', (event) => {
    if (event.source !== window || !event.data) return;

    if (event.data.type === 'TAFIM_REQ_PAGE_FORMATS') {
      extractAndSendFormats();
    }
  });

  extractAndSendFormats();
  document.addEventListener('yt-navigate-finish', () => {
    setTimeout(extractAndSendFormats, 400);
    setTimeout(extractAndSendFormats, 1200);
  });
  window.addEventListener('load', extractAndSendFormats);
  setInterval(extractAndSendFormats, 3000);
})();
