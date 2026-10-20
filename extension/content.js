// Tafim Video Sniffer & Link Click Interceptor
// Classic IDM-Style Floating Bar & Resolution Dropdown (Matching Image 2)

(function () {
  'use strict';

  const trackedVideos = new WeakSet();

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
    if (href.startsWith('javascript:') || href.startsWith('#')) return;

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
        if (!tabCapturedMedia.some(m => m.url === msg.media.url)) {
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
            if (!tabCapturedMedia.some(x => x.url === m.url)) {
              tabCapturedMedia.push(m);
            }
          }
        }
      }
    });
  }

  // Parse YouTube page DOM formats from ytInitialPlayerResponse
  function getYouTubePageFormats() {
    if (!window.location.hostname.includes('youtube.com')) return [];

    const formatsList = [];
    let playerData = null;

    if (window.ytInitialPlayerResponse) {
      playerData = window.ytInitialPlayerResponse;
    } else {
      const scripts = document.getElementsByTagName('script');
      for (let i = 0; i < scripts.length; i++) {
        const text = scripts[i].textContent;
        if (text && text.includes('ytInitialPlayerResponse')) {
          const match = text.match(/ytInitialPlayerResponse\s*=\s*({.+?});/);
          if (match) {
            try {
              playerData = JSON.parse(match[1]);
              break;
            } catch (_) {}
          }
        }
      }
    }

    if (!playerData || !playerData.streamingData) return formatsList;

    const streaming = playerData.streamingData;
    const all = [...(streaming.formats || []), ...(streaming.adaptiveFormats || [])];

    // Find best DASH audio format (prefer AAC/m4a itag 140 or webm itag 251)
    let bestAudioUrl = tabCapturedAudioUrl || '';
    if (!bestAudioUrl && streaming.adaptiveFormats) {
      for (const fmt of streaming.adaptiveFormats) {
        if (fmt && (fmt.url || fmt.signatureCipher || fmt.cipher)) {
          const m = (fmt.mimeType || '').toLowerCase();
          if (m.startsWith('audio/')) {
            if (fmt.itag === 140 || m.includes('audio/mp4')) {
              bestAudioUrl = fmt.url || '';
              break;
            } else if (!bestAudioUrl) {
              bestAudioUrl = fmt.url || '';
            }
          }
        }
      }
    }

    for (const fmt of all) {
      if (!fmt || (!fmt.url && !fmt.signatureCipher && !fmt.cipher)) continue;

      let directUrl = fmt.url || '';
      const mime = (fmt.mimeType || '').toLowerCase();
      const isAudio = mime.startsWith('audio/');
      if (isAudio) continue; // Only show videos in main download list

      let container = 'MP4 file';
      if (mime.includes('webm')) container = 'WebM file';

      let quality = fmt.qualityLabel ? `quality ${fmt.qualityLabel}` : 'quality standard';

      let sizeStr = '';
      if (fmt.contentLength) {
        const bytes = parseInt(fmt.contentLength, 10);
        if (!isNaN(bytes) && bytes > 0) {
          sizeStr = (bytes / (1024 * 1024)).toFixed(1) + ' MB';
        }
      } else if (fmt.bitrate && fmt.approxDurationMs) {
        const estBytes = (fmt.bitrate * (fmt.approxDurationMs / 1000)) / 8;
        sizeStr = (estBytes / (1024 * 1024)).toFixed(1) + ' MB';
      }

      const isDash = (fmt.itag !== 18 && fmt.itag !== 22);

      formatsList.push({
        url: directUrl,
        audioUrl: isDash ? bestAudioUrl : '',
        isDash: isDash,
        container: container,
        quality: quality,
        sizeStr: sizeStr,
        itag: fmt.itag,
        isAudio: false
      });
    }

    return formatsList;
  }

  // 3. Floating Bar & Resolution Dropdown Menu (Image 2)
  function createDownloadBtn(video) {
    if (trackedVideos.has(video)) return;
    trackedVideos.add(video);

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
      if (!video || !video.isConnected || isFullscreenActive()) {
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
      } else {
        showBarTemporarily(6000);
      }
    });

    let hideTimer = null;
    let isMouseOver = false;

    function showBarTemporarily(durationMs = 10000) {
      if (isFullscreenActive()) {
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

    video.addEventListener('play', () => showBarTemporarily(10000));
    video.addEventListener('mouseenter', () => showBarTemporarily(10000));
    video.addEventListener('mousemove', () => showBarTemporarily(10000));

    container.addEventListener('mouseenter', () => {
      isMouseOver = true;
      clearTimeout(hideTimer);
      container.style.opacity = '1';
      container.style.pointerEvents = 'auto';
    });

    container.addEventListener('mouseleave', () => {
      isMouseOver = false;
      if (!dropdown.classList.contains('active')) {
        showBarTemporarily(4000);
      }
    });

    showBarTemporarily(10000);

    // Close button click
    btnClose.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();
      container.style.display = 'none';
    });

    // Populate dropdown with resolution list matching Image 2
    function populateDropdown() {
      itemsList.innerHTML = '';

      const pageFormats = getYouTubePageFormats();
      const combined = [];

      // 1. Add captured formats from background.js (active, authenticated URLs with working signature)
      for (const cm of tabCapturedMedia) {
        if (cm && cm.url && !combined.some(x => x.url === cm.url)) {
          combined.push({
            url: cm.url,
            audioUrl: cm.audioUrl || tabCapturedAudioUrl || '',
            isDash: cm.isDash !== false,
            container: cm.container || 'MP4 file',
            quality: cm.quality || 'quality HD',
            sizeStr: cm.sizeStr || '',
            itag: cm.itag || ''
          });
        }
      }

      // 2. Add formats from page DOM that have valid direct URLs
      for (const pf of pageFormats) {
        if (!pf.url) continue;
        if (!combined.some(x => x.quality === pf.quality && x.container === pf.container)) {
          combined.push(pf);
        }
      }

      // 3. Fallback to direct video src if nothing captured yet
      if (combined.length === 0) {
        let vSrc = video.currentSrc || video.src;
        if (!vSrc || vSrc.startsWith('blob:') || vSrc.startsWith('data:')) {
          const source = video.querySelector('source');
          if (source && source.src && !source.src.startsWith('blob:')) {
            vSrc = source.src;
          }
        }
        if (vSrc && !vSrc.startsWith('blob:')) {
          combined.push({
            url: vSrc,
            audioUrl: tabCapturedAudioUrl || '',
            isDash: !!tabCapturedAudioUrl,
            container: 'MP4 file',
            quality: 'quality 720p HD',
            sizeStr: ''
          });
        }
      }

      if (combined.length === 0) {
        const emptyItem = document.createElement('div');
        emptyItem.className = 'tafim-menu-item';
        emptyItem.textContent = 'Analyzing streams... play video for 2s';
        itemsList.appendChild(emptyItem);
        return;
      }

      // Render numbered format list (e.g. "1. MP4 file, quality 720p HD (32.4 MB)")
      combined.forEach((item, index) => {
        const row = document.createElement('div');
        row.className = 'tafim-menu-item';

        const sizeLabel = item.sizeStr ? ` (${item.sizeStr})` : '';
        const itemText = `${index + 1}. ${item.container}, ${item.quality}${sizeLabel}`;

        row.innerHTML = `
          <span class="tafim-menu-item-text">${itemText}</span>
        `;

        row.addEventListener('click', (e) => {
          e.preventDefault();
          e.stopPropagation();
          dropdown.classList.remove('active');

          let baseTitle = document.title.replace(/[\\/:*?"<>|]/g, '_').trim();
          if (!baseTitle || baseTitle === 'YouTube') baseTitle = 'video';

          let ext = '.mp4';
          if (item.container.includes('WebM')) ext = '.webm';

          const finalName = baseTitle + ext;
          showToast(`🚀 Sending ${item.quality} (DASH video+audio) to Tafim...`, '#1e90ff');

          const chosenAudio = item.audioUrl || tabCapturedAudioUrl || '';
          if (item.url) {
            sendToTafim(item.url, finalName, chosenAudio, item.isDash || !!chosenAudio);
          } else if (tabCapturedMedia.length > 0) {
            sendToTafim(tabCapturedMedia[0].url, finalName, tabCapturedMedia[0].audioUrl || tabCapturedAudioUrl, true);
          } else {
            showToast('⚠️ Stream URL pending. Play video 2s then click again!', '#e67e22');
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

      let target = null;
      if (tabCapturedMedia.length > 0 && tabCapturedMedia[0].url) {
        target = tabCapturedMedia[0];
      } else {
        const pageFormats = getYouTubePageFormats();
        for (const pf of pageFormats) {
          if (pf.url) { target = pf; break; }
        }
      }

      if (target && target.url) {
        let baseTitle = document.title.replace(/[\\/:*?"<>|]/g, '_').replace(/\s*-\s*YouTube$/i, '').trim();
        if (!baseTitle || baseTitle === 'YouTube') baseTitle = 'video';
        sendToTafim(target.url, baseTitle + '.mp4', target.audioUrl || tabCapturedAudioUrl || '', true);
      } else {
        showToast('⚠️ No media stream available yet. Play video for 2s!', '#e67e22');
      }
    });

    // Close dropdown when clicking outside
    document.addEventListener('click', (e) => {
      if (!container.contains(e.target)) {
        dropdown.classList.remove('active');
      }
    });
  }

  function sendToTafim(url, filename, audioUrl, isDash) {
    if (!url) {
      showToast('⚠️ No media stream URL available', '#e74c3c');
      return;
    }

    showToast('🚀 Sending to Tafim...', '#1e90ff');

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
          cookies: document.cookie || ''
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
          cookies: document.cookie || ''
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
