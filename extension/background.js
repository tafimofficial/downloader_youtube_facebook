// Background service worker for Tafim Integration Module with DASH (Adaptive Streaming) Support

const TAFIM_API = 'http://127.0.0.1:9898/download';

const CATCH_EXTENSIONS = [
  // Compressed / Archives
  '.zip', '.rar', '.7z', '.tar', '.gz', '.bz2', '.iso', '.dmg', '.tgz', '.xz',
  // Executables & Installers
  '.exe', '.msi', '.apk', '.bin', '.deb', '.rpm', '.bat', '.cmd',
  // Media (Audio / Video)
  '.mp4', '.mkv', '.webm', '.avi', '.mov', '.flv', '.wmv', '.m4v', '.ogv', '.ts',
  '.mp3', '.wav', '.flac', '.aac', '.ogg', '.m4a', '.wma',
  // Documents
  '.pdf', '.epub', '.mobi', '.docx', '.xlsx', '.pptx',
  // Images / Disk
  '.img', '.vhd', '.vmdk', '.torrent'
];

const CATCH_MIMES = [
  'application/zip',
  'application/x-zip-compressed',
  'application/x-rar-compressed',
  'application/x-7z-compressed',
  'application/x-tar',
  'application/x-gzip',
  'application/x-msdownload',
  'application/vnd.android.package-archive',
  'application/octet-stream',
  'application/pdf',
  'video/mp4',
  'video/x-matroska',
  'video/webm',
  'video/quicktime',
  'video/x-msvideo',
  'audio/mpeg',
  'audio/x-wav',
  'audio/flac'
];

// Context Menu: "Download with Tafim"
chrome.runtime.onInstalled.addListener(() => {
  chrome.contextMenus.create({
    id: 'tafim-download-link',
    title: 'Download with Tafim',
    contexts: ['link', 'image', 'video', 'audio']
  });
});

chrome.contextMenus.onClicked.addListener((info, tab) => {
  const url = info.linkUrl || info.srcUrl;
  if (url) {
    sendDownloadToTafim(url, '', info.pageUrl || tab?.url || '');
  }
});

// Primary Download Sniffer: onDeterminingFilename
chrome.downloads.onDeterminingFilename.addListener((item, suggest) => {
  if (item.byExtensionId === chrome.runtime.id) {
    suggest();
    return;
  }

  const filename = (item.filename || '').toLowerCase();
  const url = (item.finalUrl || item.url || '').toLowerCase();
  const mime = (item.mime || '').toLowerCase();

  const extMatch = CATCH_EXTENSIONS.some(ext =>
    filename.endsWith(ext) ||
    url.endsWith(ext) ||
    url.includes(ext + '?') ||
    url.includes(ext + '&')
  );

  const mimeMatch = CATCH_MIMES.some(m => mime.includes(m));

  if (extMatch || mimeMatch) {
    console.log('[Tafim] Intercepting download:', item.filename, item.url);

    chrome.downloads.cancel(item.id, () => {
      chrome.downloads.erase({ id: item.id }, () => {});
    });

    const targetUrl = item.finalUrl || item.url;
    sendDownloadToTafim(targetUrl, item.filename, item.referrer || '');
    return;
  }

  suggest();
});

// Secondary Fallback: onCreated
chrome.downloads.onCreated.addListener((item) => {
  if (!item.url || item.url.startsWith('blob:') || item.url.startsWith('data:')) return;
  if (item.byExtensionId === chrome.runtime.id) return;

  const urlLower = item.url.toLowerCase();
  const directMatch = CATCH_EXTENSIONS.some(ext => urlLower.endsWith(ext));

  if (directMatch) {
    chrome.downloads.cancel(item.id, () => {
      chrome.downloads.erase({ id: item.id }, () => {});
    });
    sendDownloadToTafim(item.url, item.filename || '', item.referrer || '');
  }
});

function sendDownloadToTafim(url, filename, pageUrl, userAgent, cookies, audioUrl, isDash) {
  console.log('[Tafim] Sending to Tafim:', url, 'audio:', audioUrl, filename);

  const payload = {
    url: url,
    audioUrl: audioUrl || '',
    isDash: !!isDash || !!audioUrl,
    filename: filename,
    pageUrl: pageUrl,
    userAgent: userAgent || navigator.userAgent,
    cookies: cookies || ''
  };

  // 1. Send to local HTTP API (immediate dispatch to active Tafim.exe)
  return sendHttpToTafim(payload).then(httpSuccess => {
    if (httpSuccess) return true;

    // 2. Fallback: Also try Native Messaging Host (will launch Tafim if not active)
    return new Promise((resolve) => {
      try {
        chrome.runtime.sendNativeMessage('com.tafim.downloader', payload, (response) => {
          if (chrome.runtime.lastError || !response || !response.success) {
            resolve(false);
          } else {
            resolve(true);
          }
        });
      } catch (_) {
        resolve(false);
      }
    });
  });
}

function sendHttpToTafim(payload) {
  return fetch(TAFIM_API, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  })
    .then(res => res.json())
    .then(data => {
      console.log('[Tafim] HTTP API success:', data);
      return data.success === true;
    })
    .catch(err => {
      console.warn('[Tafim] HTTP API failed. Is Tafim running?', err);
      return false;
    });
}

// Media Sniffing Engine for Streaming Video & Audio (DASH support for YouTube, Facebook, etc.)
const capturedMediaByTab = new Map(); // tabId -> Map(url -> mediaItem)
const capturedAudioByTab = new Map(); // tabId -> { url, itag, mime, timestamp }

function cleanStreamUrl(rawUrl) {
  try {
    const u = new URL(rawUrl);
    u.searchParams.delete('range');
    u.searchParams.delete('bytestart');
    u.searchParams.delete('byteend');
    return u.toString();
  } catch (_) {
    return rawUrl;
  }
}

const ITAG_MAP = {
  '18': { container: 'MP4 file', quality: 'quality 360p', isDash: false },
  '22': { container: 'MP4 file', quality: 'quality 720p HD', isDash: false },
  '137': { container: 'MP4 file', quality: 'quality 1080p HD', isDash: true },
  '248': { container: 'WebM file', quality: 'quality 1080p HD', isDash: true },
  '136': { container: 'MP4 file', quality: 'quality 720p', isDash: true },
  '247': { container: 'WebM file', quality: 'quality 720p', isDash: true },
  '135': { container: 'MP4 file', quality: 'quality 480p', isDash: true },
  '244': { container: 'WebM file', quality: 'quality 480p', isDash: true },
  '134': { container: 'MP4 file', quality: 'quality 360p', isDash: true },
  '243': { container: 'WebM file', quality: 'quality 360p', isDash: true },
  '133': { container: 'MP4 file', quality: 'quality 240p', isDash: true },
  '242': { container: 'WebM file', quality: 'quality 240p', isDash: true },
  '160': { container: 'MP4 file', quality: 'quality 144p', isDash: true },
  '278': { container: 'WebM file', quality: 'quality 144p', isDash: true },
  '140': { container: 'M4A Audio file', quality: 'quality 128k', isAudio: true },
  '251': { container: 'WebM Audio file', quality: 'quality 160k', isAudio: true },
  '313': { container: 'WebM file', quality: 'quality 2160p 4K', isDash: true },
  '271': { container: 'WebM file', quality: 'quality 1440p 2K', isDash: true }
};

chrome.webRequest.onResponseStarted.addListener((details) => {
  if (details.tabId < 0) return;
  const url = details.url;

  let isVideo = false;
  let isAudio = false;
  let isDash = false;
  let container = 'MP4 file';
  let quality = 'quality 720p HD';
  let mime = 'video/mp4';
  let sizeStr = '';
  let bytes = 0;
  let itag = null;

  // 1. YouTube googlevideo.com
  if (url.includes('googlevideo.com/videoplayback')) {
    try {
      const u = new URL(url);
      itag = u.searchParams.get('itag');
      const clen = u.searchParams.get('clen');
      if (clen) {
        bytes = parseInt(clen, 10);
        if (!isNaN(bytes) && bytes > 0) {
          sizeStr = (bytes / (1024 * 1024)).toFixed(1) + ' MB';
        }
      }

      if (url.includes('mime=audio') || itag === '140' || itag === '251' || itag === '171') {
        isAudio = true;
        mime = url.includes('mime=audio%2Fwebm') ? 'audio/webm' : 'audio/mp4';
      } else {
        isVideo = true;
        if (itag && ITAG_MAP[itag]) {
          container = ITAG_MAP[itag].container;
          quality = ITAG_MAP[itag].quality;
          isDash = ITAG_MAP[itag].isDash;
        } else {
          isDash = true;
        }
      }
    } catch (_) {}
  }
  // 2. Facebook fbcdn.net
  else if (url.includes('fbcdn.net')) {
    let ct = '';
    if (details.responseHeaders) {
      for (const h of details.responseHeaders) {
        if (h.name.toLowerCase() === 'content-type') {
          ct = h.value.toLowerCase();
          break;
        }
      }
    }
    if (ct.startsWith('audio/') || url.includes('&mime=audio') || url.includes('/audio/')) {
      isAudio = true;
      mime = ct || 'audio/mp4';
    } else if (ct.startsWith('video/') || url.includes('.mp4') || url.includes('bytestart=') || url.includes('/v/t')) {
      isVideo = true;
      isDash = true; // Modern Facebook videos use DASH separate audio
      container = 'MP4 file';
      quality = 'quality HD';
      mime = ct || 'video/mp4';
    }
  }
  // 3. Generic video/audio content types
  else if (details.responseHeaders) {
    for (const h of details.responseHeaders) {
      if (h.name.toLowerCase() === 'content-type') {
        const ct = h.value.toLowerCase();
        if (ct.startsWith('video/') || ct.includes('application/dash+xml')) {
          isVideo = true;
          isDash = true;
          mime = ct;
          container = ct.includes('webm') ? 'WebM file' : 'MP4 file';
          quality = 'quality Video Stream';
          break;
        } else if (ct.startsWith('audio/')) {
          isAudio = true;
          mime = ct;
          break;
        }
      }
    }
  }

  // Handle Audio Stream discovery
  if (isAudio) {
    const streamUrl = cleanStreamUrl(url);
    capturedAudioByTab.set(details.tabId, {
      url: streamUrl,
      itag: itag,
      mime: mime,
      timestamp: Date.now()
    });

    // Retroactively update any videos on this tab with the newly discovered audio stream!
    if (capturedMediaByTab.has(details.tabId)) {
      const tabMedia = capturedMediaByTab.get(details.tabId);
      for (const mediaItem of tabMedia.values()) {
        if (mediaItem.isDash && !mediaItem.audioUrl) {
          mediaItem.audioUrl = streamUrl;
        }
      }
    }

    chrome.tabs.sendMessage(details.tabId, {
      action: 'tafimAudioDiscovered',
      audioUrl: streamUrl,
      itag: itag
    }).catch(() => {});
  }

  // Handle Video Stream discovery
  if (isVideo) {
    const streamUrl = cleanStreamUrl(url);
    if (!capturedMediaByTab.has(details.tabId)) {
      capturedMediaByTab.set(details.tabId, new Map());
    }
    const tabMedia = capturedMediaByTab.get(details.tabId);

    // Get matching audio stream for this tab if DASH
    let audioUrl = '';
    if (isDash && capturedAudioByTab.has(details.tabId)) {
      audioUrl = capturedAudioByTab.get(details.tabId).url;
    }

    if (!tabMedia.has(streamUrl)) {
      const mediaItem = {
        url: streamUrl,
        audioUrl: audioUrl,
        isDash: isDash,
        type: mime,
        container: container,
        quality: quality,
        sizeStr: sizeStr,
        bytes: bytes,
        itag: itag,
        timestamp: Date.now()
      };
      tabMedia.set(streamUrl, mediaItem);
      chrome.tabs.sendMessage(details.tabId, {
        action: 'tafimMediaDiscovered',
        media: mediaItem
      }).catch(() => {});
    } else {
      // Update existing item with audioUrl if it arrived later
      const existing = tabMedia.get(streamUrl);
      if (audioUrl && !existing.audioUrl) {
        existing.audioUrl = audioUrl;
      }
    }
  }
}, { urls: ["<all_urls>"] }, ["responseHeaders"]);

chrome.tabs.onRemoved.addListener((tabId) => {
  capturedMediaByTab.delete(tabId);
  capturedAudioByTab.delete(tabId);
});

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  if (msg.action === 'getCapturedMedia' && sender.tab) {
    const tabMedia = capturedMediaByTab.get(sender.tab.id);
    const list = tabMedia ? Array.from(tabMedia.values()) : [];
    const curAudio = capturedAudioByTab.get(sender.tab.id);
    sendResponse({ media: list, audio: curAudio ? curAudio.url : '' });
    return true;
  }
  if (msg.action === 'triggerDownload') {
    sendDownloadToTafim(msg.url, msg.filename, msg.pageUrl || sender.tab?.url || '', msg.userAgent, msg.cookies, msg.audioUrl, msg.isDash)
      .then(ok => sendResponse({ success: ok }))
      .catch(() => sendResponse({ success: false }));
    return true;
  }
});
