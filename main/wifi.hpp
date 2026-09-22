/**
 * @file wifi.hpp
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @brief 
 * @version 0.1
 * @date 2026-09-18
 * 
 * @copyright Copyright (c) 2026
 * 
 */


#ifndef __WIFI_HPP__
#define __WIFI_HPP__

#include <WiFi.h>
#include <secrets.h>
#include <WebServer.h>
#include <Arduino.h>
#include <string>

#define MAX_WIFI_CONN_RETRIES 10
#define WIFI_CONN_TIMEOUT_MS 10000

#define SERVER_PORT 2926
#define WEBUI_PORT 80


extern WebServer server;


void wifiSetup();


void wifiEventHandler();


const std::string WEBUI_TEMPLATE = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
  <title>Fan Remote</title>
  <script>
    // Apply the theme before first paint to avoid a light/dark flash.
    (function () {
      var pref = null;
      try { pref = localStorage.getItem('remote-theme'); } catch (e) {}
      var dark = pref === 'dark' || (pref !== 'light' && window.matchMedia('(prefers-color-scheme: dark)').matches);
      document.documentElement.classList.toggle('dark', dark);
      document.documentElement.style.colorScheme = dark ? 'dark' : 'light';
    })();
  </script>
  <script src="https://cdn.tailwindcss.com"></script>
  <script>tailwind.config = { darkMode: 'class' };</script>
  <style>
    @keyframes spin { to { transform: rotate(360deg); } }
    .fan-spin { animation: spin var(--spin-duration, 1s) linear infinite; }
    button { -webkit-tap-highlight-color: transparent; }
  </style>
</head>
<body class="min-h-screen bg-gradient-to-br from-slate-100 via-white to-slate-200 text-slate-900 dark:from-slate-900 dark:via-slate-800 dark:to-slate-900 dark:text-slate-100 flex items-center justify-center p-4 select-none transition-colors">

  <main class="w-full max-w-sm bg-white/80 dark:bg-slate-800/60 backdrop-blur rounded-[2.5rem] border border-slate-200 dark:border-slate-700 shadow-2xl p-6 space-y-6">

    <!-- Header -->
    <header class="flex items-center justify-between">
      <div>
        <h1 class="text-xl font-semibold tracking-tight">Ceiling Fan</h1>
        <p class="text-xs text-slate-500 dark:text-slate-400">Living Room</p>
      </div>
      <div class="flex items-center gap-3">
        <div class="flex items-center gap-2 text-xs">
          <span id="statusDot" class="h-2.5 w-2.5 rounded-full bg-slate-500"></span>
          <span id="statusText" class="text-slate-500 dark:text-slate-400">Idle</span>
        </div>
        <button id="themeBtn" type="button"
          class="h-9 w-9 flex items-center justify-center rounded-full bg-slate-200 hover:bg-slate-300 text-slate-700 dark:bg-slate-700 dark:hover:bg-slate-600 dark:text-slate-200 active:scale-95 transition">
          <!-- light -->
          <svg data-theme-icon="light" xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="1.8">
            <circle cx="12" cy="12" r="4" /><path stroke-linecap="round" d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4" />
          </svg>
          <!-- dark -->
          <svg data-theme-icon="dark" xmlns="http://www.w3.org/2000/svg" class="h-5 w-5 hidden" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="1.8">
            <path stroke-linecap="round" stroke-linejoin="round" d="M20 14.5A8 8 0 019.5 4a8 8 0 1010.5 10.5z" />
          </svg>
        </button>
      </div>
    </header>

    <!-- Fan visual -->
    <section class="flex flex-col items-center py-2">
      <div class="relative h-36 w-36 rounded-full bg-slate-100 border border-slate-200 dark:bg-slate-900/70 dark:border-slate-700 flex items-center justify-center">
        <div id="lightGlow" class="absolute inset-0 rounded-full transition-all duration-300 opacity-0"></div>
        <svg id="fanIcon" viewBox="0 0 100 100" class="relative h-24 w-24 text-sky-400">
          <g fill="currentColor">
            <path d="M50 50 C 45 30, 48 10, 60 8 C 70 7, 70 25, 50 50 Z" />
            <path d="M50 50 C 45 30, 48 10, 60 8 C 70 7, 70 25, 50 50 Z" transform="rotate(120 50 50)" />
            <path d="M50 50 C 45 30, 48 10, 60 8 C 70 7, 70 25, 50 50 Z" transform="rotate(240 50 50)" />
          </g>
          <circle cx="50" cy="50" r="8" class="fill-slate-600 dark:fill-slate-200" />
        </svg>
      </div>
      <p class="mt-3 text-sm text-slate-600 dark:text-slate-300">
        Fan: <span id="fanLabel" class="font-semibold">Off</span>
        <span class="mx-2 text-slate-300 dark:text-slate-600">|</span>
        Light: <span id="lightLabel" class="font-semibold">Off</span>
      </p>
    </section>

    <!-- Light controls -->
    <section class="grid grid-cols-2 gap-3">
      <button id="lightBtn" data-cmd="l"
        class="flex flex-col items-center justify-center gap-1 rounded-2xl py-4 bg-slate-200 hover:bg-slate-300 dark:bg-slate-700 dark:hover:bg-slate-600 active:scale-95 transition">
        <svg xmlns="http://www.w3.org/2000/svg" class="h-7 w-7" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="1.8">
          <path stroke-linecap="round" stroke-linejoin="round" d="M9 18h6M10 21h4M12 3a6 6 0 00-3.5 10.9c.6.4 1 1.1 1 1.8V16h5v-.3c0-.7.4-1.4 1-1.8A6 6 0 0012 3z" />
        </svg>
        <span class="text-sm font-medium">Light</span>
      </button>

      <button data-cmd="c"
        class="flex flex-col items-center justify-center gap-1 rounded-2xl py-4 bg-slate-200 hover:bg-slate-300 dark:bg-slate-700 dark:hover:bg-slate-600 active:scale-95 transition">
        <svg xmlns="http://www.w3.org/2000/svg" class="h-7 w-7" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="1.8">
          <path stroke-linecap="round" stroke-linejoin="round" d="M4 4v5h5M20 20v-5h-5M5.1 15A7 7 0 0018.4 17M18.9 9A7 7 0 005.6 7" />
        </svg>
        <span class="text-sm font-medium">Color Temp</span>
      </button>
    </section>

    <!-- Fan speed -->
    <section class="space-y-3">
      <h2 class="text-xs uppercase tracking-widest text-slate-500 dark:text-slate-400">Fan Speed</h2>
      <div id="speedGrid" class="grid grid-cols-3 gap-3">
        <button data-cmd="1" data-speed="1" class="speed-btn rounded-2xl py-4 text-lg font-semibold active:scale-95 transition">1</button>
        <button data-cmd="2" data-speed="2" class="speed-btn rounded-2xl py-4 text-lg font-semibold active:scale-95 transition">2</button>
        <button data-cmd="3" data-speed="3" class="speed-btn rounded-2xl py-4 text-lg font-semibold active:scale-95 transition">3</button>
        <button data-cmd="4" data-speed="4" class="speed-btn rounded-2xl py-4 text-lg font-semibold active:scale-95 transition">4</button>
        <button data-cmd="5" data-speed="5" class="speed-btn rounded-2xl py-4 text-lg font-semibold active:scale-95 transition">5</button>
        <button data-cmd="6" data-speed="6" class="speed-btn rounded-2xl py-4 text-lg font-semibold active:scale-95 transition">6</button>
      </div>
      <button data-cmd="o" data-speed="0"
        class="w-full flex items-center justify-center gap-2 rounded-2xl py-4 font-semibold text-white bg-rose-600/90 hover:bg-rose-600 active:scale-95 transition">
        <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
          <path stroke-linecap="round" stroke-linejoin="round" d="M12 3v9M6.3 6.3a8 8 0 1011.4 0" />
        </svg>
        Fan Off
      </button>
    </section>

    <footer class="text-center text-[11px] text-slate-400 dark:text-slate-500">
      Target: <span id="targetHost" class="font-mono"></span>
    </footer>
  </main>

  <script>
    // ---------- Theme ----------
    // Follows the device until the user picks light or dark; after that the pick is kept.
    const THEME_KEY = 'remote-theme';
    const systemDark = window.matchMedia('(prefers-color-scheme: dark)');

    function getStoredTheme() {
      try {
        const v = localStorage.getItem(THEME_KEY);
        return v === 'light' || v === 'dark' ? v : null;
      } catch (e) { return null; }
    }

    let userPicked = getStoredTheme() !== null;
    let currentTheme = getStoredTheme() || (systemDark.matches ? 'dark' : 'light');

    function applyTheme(theme) {
      currentTheme = theme;
      const dark = theme === 'dark';
      document.documentElement.classList.toggle('dark', dark);
      document.documentElement.style.colorScheme = theme;
      document.querySelectorAll('[data-theme-icon]').forEach((el) => el.classList.toggle('hidden', el.dataset.themeIcon !== theme));
      const label = `Switch to ${dark ? 'light' : 'dark'} mode`;
      const btn = document.getElementById('themeBtn');
      btn.title = label;
      btn.setAttribute('aria-label', label);
    }

    document.getElementById('themeBtn').addEventListener('click', () => {
      const next = currentTheme === 'dark' ? 'light' : 'dark';
      userPicked = true;
      try { localStorage.setItem(THEME_KEY, next); } catch (e) {}
      applyTheme(next);
    });

    systemDark.addEventListener('change', (e) => { if (!userPicked) applyTheme(e.matches ? 'dark' : 'light'); });
    applyTheme(currentTheme);

    // ---------- Remote ----------
    const PORT = 2926;
    // The remote is served by the device itself, so its own host is the target.
    // Falls back to localhost when opened directly from disk.
    const HOST = window.location.hostname || 'localhost';
    const BASE_URL = `http://${HOST}:${PORT}`;

    const state = { light: false, speed: 0 };

    const $ = (id) => document.getElementById(id);
    $('targetHost').textContent = `${HOST}:${PORT}`;

    // Swap between two class sets depending on a condition.
    function setClasses(el, cond, onClasses, offClasses) {
      onClasses.split(' ').forEach((c) => el.classList.toggle(c, cond));
      offClasses.split(' ').forEach((c) => el.classList.toggle(c, !cond));
    }
    const NEUTRAL = 'bg-slate-200 hover:bg-slate-300 dark:bg-slate-700 dark:hover:bg-slate-600';

    function setStatus(kind, text) {
      const colors = { idle: 'bg-slate-500', busy: 'bg-amber-400 animate-pulse', ok: 'bg-emerald-400', err: 'bg-rose-500' };
      $('statusDot').className = `h-2.5 w-2.5 rounded-full ${colors[kind]}`;
      $('statusText').textContent = text;
    }

    async function sendCommand(cmd) {
      setStatus('busy', 'Sending…');
      try {
        // no-cors: the device is on a different port (different origin),
        // so we fire the request without needing CORS headers from it.
        await fetch(`${BASE_URL}/tx?cmd=${encodeURIComponent(cmd)}`, { mode: 'no-cors', cache: 'no-store' });
        setStatus('ok', 'Sent');
        return true;
      } catch (e) {
        console.error('Command failed:', cmd, e);
        setStatus('err', 'Failed');
        return false;
      }
    }

    function render() {
      // Light
      setClasses($('lightBtn'), state.light, 'bg-amber-400 hover:bg-amber-300 text-slate-900', NEUTRAL);
      $('lightLabel').textContent = state.light ? 'On' : 'Off';
      const glow = $('lightGlow');
      glow.style.opacity = state.light ? '1' : '0';
      glow.style.boxShadow = state.light ? '0 0 60px 10px rgba(251,191,36,0.45)' : 'none';

      // Fan speed buttons
      document.querySelectorAll('#speedGrid .speed-btn').forEach((btn) => {
        const active = Number(btn.dataset.speed) === state.speed;
        setClasses(btn, active, 'bg-sky-500 hover:bg-sky-400 text-white ring-2 ring-sky-300', NEUTRAL);
      });

      // Fan animation
      const fan = $('fanIcon');
      $('fanLabel').textContent = state.speed ? `Speed ${state.speed}` : 'Off';
      if (state.speed) {
        fan.style.setProperty('--spin-duration', `${(1.6 / state.speed).toFixed(2)}s`);
        fan.classList.add('fan-spin');
        fan.classList.replace('text-slate-400', 'text-sky-400');
      } else {
        fan.classList.remove('fan-spin');
        fan.classList.replace('text-sky-400', 'text-slate-400');
      }
    }

    document.querySelectorAll('button[data-cmd]').forEach((btn) => {
      btn.addEventListener('click', async () => {
        const cmd = btn.dataset.cmd;
        if (!(await sendCommand(cmd))) return;
        if (cmd === 'l') state.light = !state.light;
        else if (cmd === 'o') state.speed = 0;
        else if (/^[1-6]$/.test(cmd)) state.speed = Number(cmd);
        render();
      });
    });

    render();
  </script>
</body>
</html>
)HTML";



#endif
