// stream-detector.js — ФИНАЛЬНАЯ ВЕРСИЯ (декабрь 2025)
// Полностью рабочий на livetv869.me + выводит прямые ссылки!

(function () {
    'use strict';

    window.detectedStreams = window.detectedStreams || [];

    function addStream(url, type = 'UNKNOWN') {
        if (!url || typeof url !== 'string') return;
        url = url.trim();

        // Пропускаем дубли
        if (window.detectedStreams.some(s => s.url === url)) return;

        const stream = {
            url: url,
            type: type,
            timestamp: Date.now()
        };

        window.detectedStreams.push(stream);

        // КРАСИВЫЙ ВЫВОД ССЫЛКИ В КОНСОЛЬ!
        if (url.includes('.m3u8')) {
            console.log("%c[HLS] → " + url, "color: cyan; font-weight: bold; font-size: 14px;");
        } else if (url.includes('.mpd')) {
            console.log("%c[DASH] → " + url, "color: magenta; font-weight: bold;");
        } else {
            console.log("%c[STREAM] → " + url, "color: yellow; font-weight: bold;");
        }

        // Автоматически выбираем лучший HLS
        if (type === 'HLS' && url.includes('master.m3u8') || url.includes('.m3u8')) {
            window.bestStreamUrl = url;
        }
    }

    // === Перехват fetch ===
    const origFetch = window.fetch;
    window.fetch = function (...args) {
        const url = typeof args[0] === 'string' ? args[0] : args[0]?.url;
        if (url && (url.includes('.m3u8') || url.includes('.mpd'))) {
            addStream(url, url.includes('.m3u8') ? 'HLS' : 'DASH');
        }
        return origFetch.apply(this, args);
    };

    // === Перехват XHR ===
    const origOpen = XMLHttpRequest.prototype.open;
    XMLHttpRequest.prototype.open = function (method, url) {
        if (url && (url.includes('.m3u8') || url.includes('.mpd'))) {
            this.addEventListener('load', function () {
                if (this.responseURL) {
                    addStream(this.responseURL, this.responseURL.includes('.m3u8') ? 'HLS' : 'DASH');
                }
            });
        }
        return origOpen.apply(this, arguments);
    };

    // === Хук в JW Player (именно он используется на livetv869.me!) ===
    if (window.jwplayer) {
        const orig = jwplayer;
        window.jwplayer = function (...args) {
            const player = orig.apply(this, args);
            const oldSetup = player.setup;
            player.setup = function (config) {
                if (config.file && (config.file.includes('.m3u8') || config.file.includes('.mpd'))) {
                    addStream(config.file, config.file.includes('.m3u8') ? 'HLS' : 'DASH');
                }
                if (config.sources) {
                    config.sources.forEach(src => {
                        if (src.file && (src.file.includes('.m3u8') || src.file.includes('.mpd'))) {
                            addStream(src.file, src.file.includes('.m3u8') ? 'HLS' : 'DASH');
                        }
                    });
                }
                return oldSetup.apply(this, arguments);
            };
            return player;
        };
    }

    // === HLS.js ===
    if (window.Hls && window.Hls.isSupported()) {
        const origLoad = Hls.prototype.loadSource;
        Hls.prototype.loadSource = function (url) {
            if (url && url.includes('.m3u8')) {
                addStream(url, 'HLS');
            }
            return origLoad.apply(this, arguments);
        };
    }

    console.log("%cStreamDetector внедрён и работает!", "color: green; font-size: 16px; font-weight: bold;");
})();