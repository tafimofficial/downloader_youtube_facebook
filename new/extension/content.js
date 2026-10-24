// Tafim Video Sniffer & Link Click Interceptor
// Classic IDM-Style Floating Bar & Resolution Dropdown (Matching Image 2)

(function () {
  'use strict';

  const trackedVideos = new WeakSet();
  let ytBridgeFormats = [];
  let ytBridgeQualityLevels = [];

  const DOWNLOADABLE_EXTS = [
    '.zip', '.rar', '.7z', '.tar', '.gz', '.bz2', '.iso', '.dmg',
    '.exe', '.msi', '.apk', '.bin', '.deb', '.rpm',
    '.mp4', '.mkv', '.webm', '.avi', '.mov', '.flv', '.wmv',
    '.mp3', '.wav', '.flac', '.aac', '.ogg',
    '.pdf', '.epub', '.docx', '.xlsx', '.pptx', '.torrent'
  ];

  // 1. Intercept direct file links
  document.addEventListener('click', (e) => {
    let target = e.target;
    while (target && target.tagName !== 'A') {
      target = target.parentElement;
    }

    if (!target || !target.href) return;

    const href = target.href.trim();
    if (href.startsWith('javascript:') || href.startsWith('#') || href.startsWith('blob:') || href.startsWith('data:') || href.startsWith('filesystem:')) return;

    // Only intercept network HTTP/HTTPS links
    if (!href.startsWith('http://') && !href.startsWith('https://')) return;

    const hrefLower = href.toLowerCase();
    const hasDownloadAttr = target.hasAttribute('download');

    const matchesExt = DOWNLOADABLE_EXTS.some(ext =>
      hrefLower.endsWith(ext) ||
      hrefLower.includes(ext + '?') ||
      hrefLower.includes(ext + '&')
    );

    if (matchesExt || hasDownloadAttr) {
      e.preventDefault();
      e.stopPropagation();

      let filename = target.getAttribute('download') || '';
      if (!filename) {
        try {
          const u = new URL(href);
          const parts = u.pathname.split('/');
          filename = parts[parts.length - 1] || 'download';
        } catch (_) {
          filename = 'download';
        }
      }

      showToast('🚀 Intercepted! Sending to Tafim...', '#1e90ff');
      sendToTafim(href, filename);
    }
  }, true);

  // 2. Track media streams discovered for this tab by background.js
  const tabCapturedMedia = [];
  let tabCapturedAudioUrl = '';

  if (typeof chrome !== 'undefined' && chrome.runtime?.onMessage) {
    chrome.runtime.onMessage.addListener((msg) => {
      if (msg.action === 'tafimAudioDiscovered' && msg.audioUrl) {
        tabCapturedAudioUrl = msg.audioUrl;
        for (const m of tabCapturedMedia) {
          if (m.isDash && !m.audioUrl) m.audioUrl = msg.audioUrl;
        }
      }
      if (msg.action === 'tafimMediaDiscovered' && msg.media) {
        if (msg.media.isDash && !msg.media.audioUrl && tabCapturedAudioUrl) {
          msg.media.audioUrl = tabCapturedAudioUrl;
        }
        if (!tabCapturedMedia.some(m => m.url === msg.media.url || (m.height && m.height === msg.media.height))) {
          tabCapturedMedia.unshift(msg.media);
        }
      }
    });

    chrome.runtime.sendMessage({ action: 'getCapturedMedia' }, (resp) => {
      if (resp) {
        if (resp.audio) tabCapturedAudioUrl = resp.audio;
        if (resp.media) {
          for (const m of resp.media) {
            if (m.isDash && !m.audioUrl && tabCapturedAudioUrl) m.audioUrl = tabCapturedAudioUrl;
            if (!tabCapturedMedia.some(x => x.url === m.url || (x.height && x.height === m.height))) {
              tabCapturedMedia.push(m);
            }
          }
        }
      }
    });
  }

  window.addEventListener('message', (event) => {
    if (event.source !== window || !event.data || event.data.type !== 'TAFIM_YT_PAGE_FORMATS') return;
    if (Array.isArray(event.data.formats)) {
      ytBridgeFormats = event.data.formats;
    }
    if (Array.isArray(event.data.qualityLevels)) {
      ytBridgeQualityLevels = event.data.qualityLevels;
    }
  });

  function isYouTubePage() {
    const h = window.location.hostname;
    return h.includes('youtube.com') || h.includes('youtu.be');
  }

  function getYouTubeWatchUrl() {
    try {
      const u = new URL(window.location.href);
      if (u.hostname.includes('youtu.be')) {
        return 'https://www.youtube.com/watch?v=' + u.pathname.replace(/\//g, '');
      }
      if (u.pathname.startsWith('/shorts/')) {
        const id = u.pathname.split('/').filter(Boolean)[1];
        if (id) return 'https://www.youtube.com/watch?v=' + id;
      }
      const v = u.searchParams.get('v');
      if (v) return 'https://www.youtube.com/watch?v=' + v;
    } catch (_) {}
    return window.location.href;
  }

  function parseQualityHeight(quality) {
    const m = String(quality || '').match(/(\d{3,4})\s*p/i);
    return m ? parseInt(m[1], 10) : 0;
  }

  function formatSize(bytes, bitrate, durationMs) {
    let n = parseInt(bytes, 10);
    if (!n && bitrate && durationMs) {
      n = (bitrate * (durationMs / 1000)) / 8;
    }
    if (!n || n <= 0) return '';
    return (n / (1024 * 1024)).toFixed(1) + ' MB';
  }

  const YOUTUBE_QL_MAP = {
    'highres': 2160,
    'hd2880': 2880,
    'hd2160': 2160,
    'hd1440': 1440,
    'hd1080': 1080,
    'hd720': 720,
    'large': 480,
    'medium': 360,
    'small': 240,
    'tiny': 144
  };

  function getYouTubeRealQualities() {
    window.postMessage({ type: 'TAFIM_REQ_PAGE_FORMATS' }, '*');

    const heightsMap = new Map(); // height -> { height, sizeStr }

    // 1. From ytBridgeFormats
    for (const fmt of ytBridgeFormats) {
      if (!fmt || fmt.isAudio) continue;
      const mime = (fmt.mimeType || '').toLowerCase();
      if (mime.startsWith('audio/')) continue;

      const height = fmt.height || parseQualityHeight(fmt.qualityLabel || fmt.quality);
      if (height && height >= 144) {
        const sz = formatSize(fmt.contentLength, fmt.bitrate, fmt.approxDurationMs);
        if (!heightsMap.has(height) || (sz && !heightsMap.get(height).sizeStr)) {
          heightsMap.set(height, { height, sizeStr: sz });
        }
      }
    }

    // 2. From ytBridgeQualityLevels
    for (const ql of ytBridgeQualityLevels) {
      const h = YOUTUBE_QL_MAP[ql];
      if (h && !heightsMap.has(h)) {
        heightsMap.set(h, { height: h, sizeStr: '' });
      }
    }

    // 3. Fallback standard heights if playing
    if (heightsMap.size === 0) {
      [1080, 720, 480, 360, 240, 144].forEach(h => {
        heightsMap.set(h, { height: h, sizeStr: '' });
      });
    }

    const sorted = Array.from(heightsMap.values()).sort((a, b) => b.height - a.height);
    return sorted.map(item => ({
      url: getYouTubeWatchUrl(),
      isDash: true,
      container: 'MP4 file',
      quality: item.height >= 720 ? `quality ${item.height}p HD` : `quality ${item.height}p`,
      sizeStr: item.sizeStr || '',
      height: item.height
    }));
  }

  function uniqueGenericQualities(items) {
    const byHeight = new Map();
    for (const item of items) {
      if (!item) continue;
      const height = item.height || parseQualityHeight(item.quality);
      if (!height || height <= 0) continue;
      if (!byHeight.has(height)) {
        byHeight.set(height, {
          ...item,
          height: height,
          container: (item.container || 'MP4 file').includes('WebM') ? 'WebM file' : 'MP4 file',
          quality: height >= 720 ? `quality ${height}p HD` : `quality ${height}p`
        });
      }
    }
    return Array.from(byHeight.values()).sort((a, b) => b.height - a.height);
  }

  // 3. Floating Bar & Resolution Dropdown Menu
  function createDownloadBtn(video) {
    if (trackedVideos.has(video)) return;
    trackedVideos.add(video);

    let isBarDismissed = false;

    const container = document.createElement('div');
    container.className = 'tafim-video-container';

    container.innerHTML = `
      <div class="tafim-video-bar">
        <span class="tafim-bar-play">▶</span>
        <span class="tafim-bar-label">Download this video</span>
        <span class="tafim-bar-divider"></span>
        <span class="tafim-bar-btn tafim-bar-help" title="About">?</span>
        <span class="tafim-bar-btn tafim-bar-close" title="Close">✕</span>
      </div>
      <div class="tafim-dropdown-menu">
        <div class="tafim-menu-header">Download all</div>
        <div class="tafim-menu-divider"></div>
        <div class="tafim-menu-items-list"></div>
      </div>
    `;

    document.body.appendChild(container);

    const bar = container.querySelector('.tafim-video-bar');
    const dropdown = container.querySelector('.tafim-dropdown-menu');
    const itemsList = container.querySelector('.tafim-menu-items-list');
    const btnClose = container.querySelector('.tafim-bar-close');
    const btnHeader = container.querySelector('.tafim-menu-header');

    function isFullscreenActive() {
      return !!(
        document.fullscreenElement ||
        document.webkitFullscreenElement ||
        document.mozFullScreenElement ||
        document.msFullscreenElement ||
        (video.clientWidth >= window.innerWidth - 10 && video.clientHeight >= window.innerHeight - 10)
      );
    }

    function updatePosition() {
      if (isBarDismissed || !video || !video.isConnected || isFullscreenActive()) {
        container.style.display = 'none';
        return;
      }

      const rect = video.getBoundingClientRect();
      if (rect.width < 120 || rect.height < 80 || rect.top > window.innerHeight || rect.bottom < 0) {
        container.style.display = 'none';
        return;
      }

      container.style.display = 'inline-block';
      container.style.top = `${window.scrollY + rect.top + 8}px`;
      container.style.left = `${window.scrollX + rect.right - bar.offsetWidth - 12}px`;
    }

    updatePosition();
    window.addEventListener('scroll', updatePosition, { passive: true });
    window.addEventListener('resize', updatePosition, { passive: true });
    document.addEventListener('fullscreenchange', () => {
      if (isFullscreenActive()) {
        container.style.display = 'none';
      } else if (!isBarDismissed) {
        showBarTemporarily(6000);
      }
    });

    let hideTimer = null;
    let isMouseOver = false;

    function showBarTemporarily(durationMs = 10000) {
      if (isBarDismissed || isFullscreenActive()) {
        container.style.display = 'none';
        return;
      }

      updatePosition();
      container.style.display = 'inline-block';
      container.style.opacity = '1';
      container.style.pointerEvents = 'auto';

      clearTimeout(hideTimer);
      if (!isMouseOver && !dropdown.classList.contains('active')) {
        hideTimer = setTimeout(() => {
          if (!isMouseOver && !dropdown.classList.contains('active')) {
            container.style.opacity = '0';
            container.style.pointerEvents = 'none';
          }
        }, durationMs);
      }
    }

    video.addEventListener('play', () => {
      if (!isBarDismissed) showBarTemporarily(10000);
    });
    video.addEventListener('mouseenter', () => {
      if (!isBarDismissed) showBarTemporarily(10000);
    });
    video.addEventListener('mousemove', () => {
      if (!isBarDismissed) showBarTemporarily(10000);
    });

    // Reset dismissed state when a new video is loaded
    video.addEventListener('loadeddata', () => {
      isBarDismissed = false;
      showBarTemporarily(8000);
    });

    document.addEventListener('yt-navigate-finish', () => {
      isBarDismissed = false;
      dropdown.classList.remove('active');
      setTimeout(() => showBarTemporarily(8000), 500);
    });

    container.addEventListener('mouseenter', () => {
      if (isBarDismissed) return;
      isMouseOver = true;
      clearTimeout(hideTimer);
      container.style.opacity = '1';
      container.style.pointerEvents = 'auto';
    });

    container.addEventListener('mouseleave', () => {
      isMouseOver = false;
      if (!dropdown.classList.contains('active') && !isBarDismissed) {
        showBarTemporarily(4000);
      }
    });

    showBarTemporarily(10000);

    // Close button (X) click: Dismiss bar & close dropdown completely
    btnClose.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();
      isBarDismissed = true;
      dropdown.classList.remove('active');
      container.style.display = 'none';
      container.style.opacity = '0';
      container.style.pointerEvents = 'none';
      clearTimeout(hideTimer);
    });

    // Populate dropdown with strictly unique resolutions matching Image 2
    function populateDropdown() {
      itemsList.innerHTML = '';

      let unique = [];

      if (isYouTubePage()) {
        unique = getYouTubeRealQualities();
      } else {
        const combined = [];
        for (const cm of tabCapturedMedia) {
          if (!cm) continue;
          const height = cm.height || parseQualityHeight(cm.quality);
          if (height) combined.push(cm);
        }
        unique = uniqueGenericQualities(combined);

        if (unique.length === 0) {
          let vSrc = video.currentSrc || video.src;
          if (!vSrc || vSrc.startsWith('blob:') || vSrc.startsWith('data:')) {
            const source = video.querySelector('source');
            if (source && source.src && !source.src.startsWith('blob:')) {
              vSrc = source.src;
            }
          }
          if (vSrc && !vSrc.startsWith('blob:')) {
            unique = [{
              url: vSrc,
              audioUrl: tabCapturedAudioUrl || '',
              isDash: !!tabCapturedAudioUrl,
              container: 'MP4 file',
              quality: 'quality original',
              sizeStr: '',
              height: 0
            }];
          }
        }
      }

      if (unique.length === 0) {
        const emptyItem = document.createElement('div');
        emptyItem.className = 'tafim-menu-item';
        emptyItem.textContent = isYouTubePage()
          ? 'Reading video qualities...'
          : 'Analyzing streams... play video for 2s';
        itemsList.appendChild(emptyItem);
        return;
      }

      unique.forEach((item, index) => {
        const row = document.createElement('div');
        row.className = 'tafim-menu-item';

        const sizeLabel = item.sizeStr ? ` (${item.sizeStr})` : '';
        const itemText = `${index + 1}. ${item.container}, ${item.quality}${sizeLabel}`;

        row.innerHTML = `<span class="tafim-menu-item-text">${itemText}</span>`;

        row.addEventListener('click', (e) => {
          e.preventDefault();
          e.stopPropagation();
          dropdown.classList.remove('active');

          let baseTitle = document.title.replace(/[\\/:*?"<>|]/g, '_').replace(/\s*-\s*YouTube$/i, '').trim();
          if (!baseTitle || baseTitle === 'YouTube') baseTitle = 'video';

          const finalName = baseTitle + '.mp4';
          const height = item.height || parseQualityHeight(item.quality);
          showToast(`Sending ${item.quality} to Tafim...`, '#1e90ff');

          if (isYouTubePage()) {
            sendToTafim(getYouTubeWatchUrl(), finalName, '', false, height);
          } else {
            const chosenAudio = item.audioUrl || tabCapturedAudioUrl || '';
            sendToTafim(item.url, finalName, chosenAudio, item.isDash || !!chosenAudio, height);
          }
        });

        itemsList.appendChild(row);
      });
    }

    // Toggle dropdown on bar click
    bar.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();

      const isActive = dropdown.classList.contains('active');
      if (isActive) {
        dropdown.classList.remove('active');
      } else {
        populateDropdown();
        dropdown.classList.add('active');
        clearTimeout(hideTimer);
      }
    });

    // Download All click
    btnHeader.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();
      dropdown.classList.remove('active');

      let baseTitle = document.title.replace(/[\\/:*?"<>|]/g, '_').replace(/\s*-\s*YouTube$/i, '').trim();
      if (!baseTitle || baseTitle === 'YouTube') baseTitle = 'video';

      if (isYouTubePage()) {
        sendToTafim(getYouTubeWatchUrl(), baseTitle + '.mp4', '', false, 0);
      } else if (tabCapturedMedia.length > 0 && tabCapturedMedia[0].url) {
        const target = tabCapturedMedia[0];
        sendToTafim(target.url, baseTitle + '.mp4', target.audioUrl || tabCapturedAudioUrl || '', true, target.height || parseQualityHeight(target.quality));
      } else {
        showToast('No media stream available yet. Play video for 2s.', '#e67e22');
      }
    });

    // Close dropdown when clicking outside
    document.addEventListener('click', (e) => {
      if (!container.contains(e.target)) {
        dropdown.classList.remove('active');
      }
    });
  }

  function sendToTafim(url, filename, audioUrl, isDash, qualityHeight) {
    if (!url) {
      showToast('No media stream URL available', '#e74c3c');
      return;
    }

    showToast('Sending to Tafim...', '#1e90ff');

    const tryDirectHttp = () => {
      fetch('http://127.0.0.1:9898/download', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          url: url,
          audioUrl: audioUrl || '',
          isDash: !!isDash || !!audioUrl,
          filename: filename,
          pageUrl: window.location.href,
          userAgent: navigator.userAgent,
          cookies: document.cookie || '',
          qualityHeight: qualityHeight || 0
        })
      })
      .then(r => r.json())
      .then(d => {
        if (d && d.success) {
          showToast('✅ Download started in Tafim!', '#2ecc71');
        } else {
          showToast('⚠️ Could not start download. Is Tafim running?', '#e67e22');
        }
      })
      .catch(() => {
        showToast('⚠️ Could not connect to Tafim (127.0.0.1:9898). Please open Tafim.exe!', '#e74c3c');
      });
    };

    if (typeof chrome !== 'undefined' && chrome.runtime?.sendMessage) {
      try {
        chrome.runtime.sendMessage({
          action: 'triggerDownload',
          url: url,
          audioUrl: audioUrl || '',
          isDash: !!isDash || !!audioUrl,
          filename: filename,
          pageUrl: window.location.href,
          userAgent: navigator.userAgent,
          cookies: document.cookie || '',
          qualityHeight: qualityHeight || 0
        }, (res) => {
          if (chrome.runtime.lastError || !res || !res.success) {
            tryDirectHttp();
          } else {
            showToast('✅ Download started in Tafim!', '#2ecc71');
          }
        });
        return;
      } catch (_) {
        tryDirectHttp();
        return;
      }
    }

    tryDirectHttp();
  }

  function showToast(message, color = '#1e90ff') {
    const existing = document.querySelector('.tafim-toast-notification');
    if (existing) existing.remove();

    const toast = document.createElement('div');
    toast.className = 'tafim-toast-notification';
    toast.style.borderLeftColor = color;
    toast.textContent = message;
    document.body.appendChild(toast);

    setTimeout(() => {
      if (toast.isConnected) toast.remove();
    }, 3500);
  }

  function scanMedia() {
    const videos = document.querySelectorAll('video');
    videos.forEach(v => {
      if (v.src || v.currentSrc || v.querySelector('source') || v.duration > 0) {
        createDownloadBtn(v);
      } else {
        v.addEventListener('play', () => createDownloadBtn(v), { once: true });
        v.addEventListener('loadeddata', () => createDownloadBtn(v), { once: true });
      }
    });
  }

  scanMedia();
  const observer = new MutationObserver(() => scanMedia());
  observer.observe(document.body || document.documentElement, {
    childList: true,
    subtree: true
  });
})();
