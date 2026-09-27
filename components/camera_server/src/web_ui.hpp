#pragma once

#include <string_view>

namespace camera_server {

inline constexpr std::string_view index_html = R"html(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="color-scheme" content="dark light">
<title>{{name}}</title>
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'%3E%3Crect width='24' height='24' rx='6' fill='%236d7cff'/%3E%3Ccircle cx='12' cy='12.5' r='4' fill='none' stroke='white' stroke-width='2'/%3E%3C/svg%3E">
<style>
:root{
  --bg:#0a0c11;--surface:#12151d;--surface-2:#191d28;--surface-3:#222736;
  --border:rgba(255,255,255,.07);--border-strong:rgba(255,255,255,.14);
  --text:#e8ebf2;--muted:#8a92a6;--accent:#6d7cff;--accent-soft:rgba(109,124,255,.16);
  --ok:#2fd28a;--warn:#f5b93d;--danger:#ff5d6c;
  --radius:16px;--shadow:0 10px 30px rgba(0,0,0,.35);
  font-family:ui-sans-serif,system-ui,-apple-system,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;
}
@media (prefers-color-scheme:light){
  :root{
    --bg:#f3f5fa;--surface:#fff;--surface-2:#f5f7fb;--surface-3:#e9edf5;
    --border:rgba(15,23,42,.08);--border-strong:rgba(15,23,42,.16);
    --text:#141925;--muted:#667085;--accent:#4f5fef;--accent-soft:rgba(79,95,239,.1);
    --shadow:0 10px 30px rgba(15,23,42,.08);
  }
}
*{box-sizing:border-box}
html,body{margin:0;height:100%}
body{background:var(--bg);color:var(--text);font-size:14px;line-height:1.45;-webkit-font-smoothing:antialiased}
button,select,input{font:inherit;color:inherit}
svg{width:18px;height:18px;flex:none;fill:none;stroke:currentColor;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}
[hidden]{display:none!important}

.topbar{position:sticky;top:0;z-index:20;display:flex;align-items:center;gap:16px;padding:12px 20px;
  background:color-mix(in srgb,var(--bg) 82%,transparent);backdrop-filter:blur(14px);border-bottom:1px solid var(--border)}
.brand{display:flex;align-items:center;gap:12px;margin-right:auto;min-width:0}
.logo{display:grid;place-items:center;width:38px;height:38px;border-radius:12px;color:#fff;
  background:linear-gradient(135deg,var(--accent),#9b6dff);box-shadow:0 6px 18px rgba(109,124,255,.35)}
.brand strong{display:block;font-size:16px;letter-spacing:-.01em}
.brand small{display:block;color:var(--muted);font-size:12px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.pills{display:flex;gap:8px}
.pill{display:inline-flex;align-items:center;gap:7px;height:30px;padding:0 12px;border-radius:999px;
  background:var(--surface);border:1px solid var(--border);color:var(--muted);font-size:12px;font-weight:600;white-space:nowrap}
.dot{width:8px;height:8px;border-radius:50%;background:var(--muted)}
.pill.online .dot{background:var(--ok)}
.pill.live{color:var(--text)}
.pill.live .dot{background:var(--danger);box-shadow:0 0 0 0 rgba(255,93,108,.6);animation:pulse 1.6s infinite}
.pill.offline .dot{background:var(--danger)}
@keyframes pulse{70%{box-shadow:0 0 0 8px rgba(255,93,108,0)}100%{box-shadow:0 0 0 0 rgba(255,93,108,0)}}

.icon-btn{display:inline-grid;place-items:center;width:38px;height:38px;border-radius:12px;cursor:pointer;
  background:var(--surface);border:1px solid var(--border);transition:.15s}
.icon-btn:hover{border-color:var(--border-strong);background:var(--surface-2)}
#panel-toggle{display:none}

.layout{display:grid;grid-template-columns:minmax(0,1fr) 360px;gap:20px;padding:20px;max-width:1500px;margin:0 auto}
.stage{display:flex;flex-direction:column;gap:20px;min-width:0}

.viewer{position:relative;aspect-ratio:4/3;max-height:calc(100vh - 220px);width:100%;border-radius:var(--radius);overflow:hidden;
  background:radial-gradient(ellipse at center,#1a1f2c 0%,#07090d 75%);border:1px solid var(--border);box-shadow:var(--shadow)}
.viewer img{position:absolute;inset:0;width:100%;height:100%;object-fit:contain;opacity:0;transition:opacity .3s}
.viewer.has-frame img{opacity:1}
.viewer:fullscreen{border-radius:0;max-height:none;border:0}
.placeholder{position:absolute;inset:0;display:grid;place-content:center;justify-items:center;gap:14px;text-align:center;color:#c9cfdc;padding:24px}
.viewer.has-frame .placeholder{display:none}
.placeholder h2{margin:0;font-size:18px;font-weight:600;color:#fff}
.placeholder p{margin:0;color:#8a92a6;max-width:320px}
.play-big{display:grid;place-items:center;width:72px;height:72px;border-radius:50%;border:0;cursor:pointer;color:#fff;
  background:linear-gradient(135deg,var(--accent),#9b6dff);box-shadow:0 12px 30px rgba(109,124,255,.45);transition:transform .15s}
.play-big:hover{transform:scale(1.06)}
.play-big svg{width:28px;height:28px;margin-left:3px;fill:currentColor}
.spinner{width:36px;height:36px;border-radius:50%;border:3px solid rgba(255,255,255,.15);border-top-color:#fff;animation:spin .8s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}

.toolbar{position:absolute;left:50%;bottom:16px;transform:translateX(-50%);display:flex;gap:6px;padding:6px;border-radius:14px;
  background:rgba(10,12,17,.62);backdrop-filter:blur(12px);border:1px solid rgba(255,255,255,.08);color:#fff;transition:opacity .25s}
.viewer.has-frame:not(:hover) .toolbar{opacity:0}
.toolbar button{display:inline-flex;align-items:center;gap:8px;height:38px;padding:0 12px;border:0;border-radius:10px;background:transparent;color:#fff;cursor:pointer;font-weight:600}
.toolbar button:hover{background:rgba(255,255,255,.12)}
.toolbar button:disabled{opacity:.5;cursor:default}
.toolbar .sep{width:1px;margin:6px 2px;background:rgba(255,255,255,.14)}
.flash{position:absolute;inset:0;background:#fff;opacity:0;pointer-events:none}
.flash.go{animation:flash .45s ease-out}
@keyframes flash{from{opacity:.75}to{opacity:0}}
.badge{position:absolute;top:14px;left:14px;display:flex;gap:6px}
.badge span{padding:4px 9px;border-radius:8px;background:rgba(10,12,17,.6);backdrop-filter:blur(8px);color:#fff;font-size:12px;font-weight:600}
.viewer:not(.has-frame) .badge{display:none}

.stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:12px}
.stat{padding:14px 16px;border-radius:14px;background:var(--surface);border:1px solid var(--border)}
.stat span{display:block;color:var(--muted);font-size:12px;margin-bottom:4px}
.stat strong{font-size:18px;font-weight:650;letter-spacing:-.01em;font-variant-numeric:tabular-nums}

.card{border-radius:var(--radius);background:var(--surface);border:1px solid var(--border)}
.card-head{display:flex;align-items:center;justify-content:space-between;padding:14px 16px 0}
.card-head h3{margin:0;font-size:14px}
.card-head button{border:0;background:none;color:var(--muted);cursor:pointer;font-size:12px}
.gallery{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:10px;padding:14px 16px 16px}
.shot{position:relative;display:block;aspect-ratio:4/3;border-radius:10px;overflow:hidden;background:var(--surface-3);border:1px solid var(--border)}
.shot img{width:100%;height:100%;object-fit:cover;transition:transform .25s}
.shot:hover img{transform:scale(1.05)}
.shot span{position:absolute;inset:auto 0 0;display:flex;align-items:center;justify-content:space-between;padding:6px 8px;
  background:linear-gradient(transparent,rgba(0,0,0,.7));color:#fff;font-size:11px;font-weight:600}
.shot span svg{width:14px;height:14px}

.panel{align-self:start;position:sticky;top:82px;max-height:calc(100vh - 102px);overflow:auto;display:flex;flex-direction:column;gap:14px;scrollbar-width:thin}
.panel-head{display:flex;align-items:center;justify-content:space-between;padding:4px 4px 2px}
.panel-head h2{margin:0;font-size:15px}
.panel-head .icon-btn{display:none}
#controls{display:flex;flex-direction:column;gap:12px}
.section{border-radius:14px;background:var(--surface);border:1px solid var(--border)}
.section summary{display:flex;align-items:center;gap:10px;padding:14px 16px;cursor:pointer;list-style:none;font-weight:600;user-select:none}
.section summary::-webkit-details-marker{display:none}
.section summary .ico{display:grid;place-items:center;width:28px;height:28px;border-radius:8px;background:var(--accent-soft);color:var(--accent)}
.section summary .ico svg{width:15px;height:15px}
.section summary .chev{margin-left:auto;color:var(--muted);transition:transform .2s}
.section[open] summary .chev{transform:rotate(180deg)}
.section-body{display:flex;flex-direction:column;gap:6px;padding:0 16px 16px}

.row{display:flex;flex-direction:column;gap:10px;padding:14px 0;border-top:1px solid var(--border);transition:opacity .2s}
.row:first-child{border-top:0}
.row.disabled{opacity:.4;pointer-events:none}
.row.busy{opacity:.6}
.row-head{display:flex;align-items:center;justify-content:space-between;gap:12px}
.row-head label{font-weight:500}
.row-head output{min-width:46px;padding:2px 8px;border-radius:6px;background:var(--surface-3);color:var(--muted);font-size:12px;font-weight:600;text-align:center;font-variant-numeric:tabular-nums}
.row.inline{flex-direction:row;align-items:center;justify-content:space-between}

input[type=range]{-webkit-appearance:none;appearance:none;width:100%;height:22px;background:transparent;cursor:pointer;--p:50%}
input[type=range]::-webkit-slider-runnable-track{height:6px;border-radius:6px;background:linear-gradient(90deg,var(--accent) var(--p),var(--surface-3) var(--p))}
input[type=range]::-moz-range-track{height:6px;border-radius:6px;background:linear-gradient(90deg,var(--accent) var(--p),var(--surface-3) var(--p))}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;margin-top:-6px;border-radius:50%;background:#fff;border:0;box-shadow:0 1px 4px rgba(0,0,0,.35),0 0 0 4px var(--accent-soft)}
input[type=range]::-moz-range-thumb{width:18px;height:18px;border-radius:50%;background:#fff;border:0;box-shadow:0 1px 4px rgba(0,0,0,.35),0 0 0 4px var(--accent-soft)}

select{width:100%;height:38px;padding:0 36px 0 12px;border-radius:10px;border:1px solid var(--border-strong);background:var(--surface-2);cursor:pointer;
  -webkit-appearance:none;appearance:none;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%238a92a6' stroke-width='2'%3E%3Cpath d='m6 9 6 6 6-6'/%3E%3C/svg%3E");
  background-repeat:no-repeat;background-position:right 10px center;background-size:16px}
select:focus-visible,input:focus-visible,button:focus-visible{outline:2px solid var(--accent);outline-offset:2px}

.switch{position:relative;flex:none;width:42px;height:24px}
.switch input{position:absolute;inset:0;margin:0;opacity:0;cursor:pointer;z-index:1}
.switch i{position:absolute;inset:0;border-radius:999px;background:var(--surface-3);border:1px solid var(--border-strong);transition:.2s}
.switch i::after{content:"";position:absolute;top:2px;left:2px;width:18px;height:18px;border-radius:50%;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.3);transition:transform .2s}
.switch input:checked+i{background:var(--accent);border-color:var(--accent)}
.switch input:checked+i::after{transform:translateX(18px)}
.switch input:focus-visible+i{outline:2px solid var(--accent);outline-offset:2px}

.toasts{position:fixed;right:20px;bottom:20px;z-index:50;display:flex;flex-direction:column;gap:8px}
.toast{display:flex;align-items:center;gap:10px;padding:11px 14px;border-radius:12px;background:var(--surface-2);border:1px solid var(--border-strong);
  box-shadow:var(--shadow);font-weight:500;animation:toast-in .25s ease-out}
.toast svg{color:var(--ok)}
.toast.error svg{color:var(--danger)}
.toast.out{opacity:0;transform:translateY(8px);transition:.25s}
@keyframes toast-in{from{opacity:0;transform:translateY(8px)}}
.scrim{display:none}
.hint{color:var(--muted);font-size:12px;text-align:center;padding:4px}
kbd{padding:1px 6px;border-radius:5px;border:1px solid var(--border-strong);background:var(--surface-2);font-family:inherit;font-size:11px}

.lang{display:inline-flex;padding:3px;border-radius:11px;background:var(--surface);border:1px solid var(--border)}
.lang button{height:30px;min-width:38px;padding:0 8px;border:0;border-radius:8px;background:none;color:var(--muted);font-size:12px;font-weight:700;cursor:pointer;transition:.15s}
.lang button:hover{color:var(--text)}
.lang button[aria-pressed=true]{background:var(--accent-soft);color:var(--accent)}

@media (max-width:1000px){
  .layout{grid-template-columns:1fr;padding:14px;gap:14px}
  #panel-toggle{display:inline-grid}
  .pills .pill.opt{display:none}
  .panel{position:fixed;z-index:40;inset:auto 0 0 0;top:auto;max-height:78vh;padding:14px;border-radius:20px 20px 0 0;background:var(--bg);
    border-top:1px solid var(--border-strong);box-shadow:0 -20px 40px rgba(0,0,0,.35);transform:translateY(105%);transition:transform .3s cubic-bezier(.2,.8,.2,1)}
  .panel.open{transform:none}
  .panel-head .icon-btn{display:inline-grid}
  .scrim{position:fixed;inset:0;z-index:30;display:block;background:rgba(0,0,0,.45);opacity:0;pointer-events:none;transition:opacity .3s}
  .scrim.open{opacity:1;pointer-events:auto}
  .viewer{max-height:none}
  .toolbar .label{display:none}
  .viewer.has-frame .toolbar{opacity:1!important}
  .hint{display:none}
}
</style>
</head>
<body data-name="{{name}}">
<header class="topbar">
  <div class="brand">
    <div class="logo"><svg viewBox="0 0 24 24"><path d="M14.5 4h-5L7 7H4a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2V9a2 2 0 0 0-2-2h-3z"/><circle cx="12" cy="13" r="3"/></svg></div>
    <div><strong>{{name}}</strong><small id="subtitle">…</small></div>
  </div>
  <div class="pills">
    <span class="pill" id="status"><i class="dot"></i><span></span></span>
    <span class="pill opt" id="fps-pill">0 fps</span>
    <span class="pill opt" id="res-pill">—</span>
  </div>
  <div class="lang" role="group" data-i18n-aria="language">
    <button data-lang="pt" aria-pressed="false">PT</button>
    <button data-lang="en" aria-pressed="false">EN</button>
  </div>
  <button class="icon-btn" id="panel-toggle" data-i18n-aria="open_settings"><svg viewBox="0 0 24 24"><path d="M4 21v-7M4 10V3M12 21v-9M12 8V3M20 21v-5M20 12V3M1 14h6M9 8h6M17 16h6"/></svg></button>
</header>

<main class="layout">
  <section class="stage">
    <div class="viewer" id="viewer">
      <img id="stream" data-i18n-alt="stream_alt">
      <div class="flash" id="flash"></div>
      <div class="badge"><span id="badge-res">—</span><span id="badge-fps">0 fps</span></div>
      <div class="placeholder" id="placeholder">
        <button class="play-big" id="play-big" data-i18n-aria="start_stream"><svg viewBox="0 0 24 24"><path d="M7 4v16l13-8z"/></svg></button>
        <h2 id="ph-title"></h2>
        <p id="ph-text"></p>
      </div>
      <div class="toolbar">
        <button id="play" data-i18n-title="play_title"><svg viewBox="0 0 24 24" id="play-icon"></svg><span class="label" id="play-label"></span></button>
        <span class="sep"></span>
        <button id="snap" data-i18n-title="snap_title"><svg viewBox="0 0 24 24"><path d="M14.5 4h-5L7 7H4a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2V9a2 2 0 0 0-2-2h-3z"/><circle cx="12" cy="13" r="3"/></svg><span class="label" data-i18n="snap"></span></button>
        <button id="fullscreen" data-i18n-title="fullscreen_title"><svg viewBox="0 0 24 24"><path d="M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3"/></svg></button>
      </div>
    </div>

    <div class="stats">
      <div class="stat"><span data-i18n="stat_uptime"></span><strong id="st-uptime">—</strong></div>
      <div class="stat"><span data-i18n="stat_viewers"></span><strong id="st-streams">—</strong></div>
      <div class="stat"><span data-i18n="stat_frames"></span><strong id="st-frames">—</strong></div>
      <div class="stat"><span data-i18n="stat_bytes"></span><strong id="st-bytes">—</strong></div>
      <div class="stat"><span data-i18n="stat_signal"></span><strong id="st-rssi">—</strong></div>
      <div class="stat" data-i18n-title="memory_hint"><span data-i18n="stat_memory"></span><strong id="st-heap">—</strong></div>
    </div>

    <div class="card" id="shots" hidden>
      <div class="card-head"><h3 data-i18n="shots_title"></h3><button id="clear-shots" data-i18n="clear"></button></div>
      <div class="gallery" id="gallery"></div>
    </div>
  </section>

  <aside class="panel" id="panel">
    <div class="panel-head">
      <h2 data-i18n="settings_title"></h2>
      <button class="icon-btn" id="panel-close" data-i18n-aria="close_settings"><svg viewBox="0 0 24 24"><path d="M18 6 6 18M6 6l12 12"/></svg></button>
    </div>
    <div id="controls"></div>
    <div class="hint" data-i18n-html="shortcuts"></div>
  </aside>
</main>
<div class="scrim" id="scrim"></div>
<div class="toasts" id="toasts"></div>

<script>
const $ = (selector) => document.querySelector(selector);
const ICONS = {
  play: '<path d="M7 4v16l13-8z" fill="currentColor"/>',
  pause: '<rect x="6" y="4" width="4" height="16" rx="1" fill="currentColor"/><rect x="14" y="4" width="4" height="16" rx="1" fill="currentColor"/>',
  image: '<rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="9" cy="9" r="2"/><path d="m21 15-5-5L5 21"/>',
  sun: '<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/>',
  drop: '<path d="M12 2.7 6.3 8.3a8 8 0 1 0 11.4 0z"/>',
  cpu: '<rect x="5" y="5" width="14" height="14" rx="2"/><path d="M9 1v4M15 1v4M9 19v4M15 19v4M1 9h4M1 15h4M19 9h4M19 15h4"/>',
  chevron: '<path d="m6 9 6 6 6-6"/>',
  check: '<path d="M20 6 9 17l-5-5"/>',
  alert: '<circle cx="12" cy="12" r="10"/><path d="M12 8v4M12 16h.01"/>',
  download: '<path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M7 10l5 5 5-5M12 15V3"/>',
};
const svg = (name) => `<svg viewBox="0 0 24 24">${ICONS[name]}</svg>`;

const I18N = {
  pt: {
    locale: 'pt-BR', language: 'Idioma',
    connecting: 'Conectando…', camera: 'Câmera', sensor: 'Sensor {0}',
    status_connecting: 'Conectando', status_online: 'Conectado', status_live: 'Ao vivo', status_offline: 'Offline',
    open_settings: 'Abrir ajustes', close_settings: 'Fechar ajustes', settings_title: 'Ajustes da câmera',
    stream_alt: 'Transmissão da câmera', start_stream: 'Iniciar transmissão',
    paused_title: 'Transmissão pausada', paused_text: 'Inicie a transmissão ao vivo ou capture uma foto a qualquer momento.',
    waiting_title: 'Conectando…', waiting_text: 'Aguardando o primeiro quadro da câmera.',
    lost_title: 'Transmissão interrompida', lost_text: 'A conexão com a câmera foi perdida ou o limite de espectadores foi atingido.',
    play: 'Iniciar', pause: 'Pausar', play_title: 'Iniciar/pausar (espaço)',
    snap: 'Foto', snap_title: 'Capturar foto (S)', fullscreen_title: 'Tela cheia (F)',
    stat_uptime: 'Tempo ativo', stat_viewers: 'Espectadores', stat_frames: 'Quadros enviados', stat_bytes: 'Dados enviados',
    stat_signal: 'Sinal Wi-Fi', stat_memory: 'Memória livre', memory_hint: 'RAM interna · PSRAM',
    signal: ['Ótimo', 'Bom', 'Regular', 'Fraco'],
    shots_title: 'Fotos desta sessão', clear: 'Limpar', download: 'Baixar {0}',
    shortcuts: '<kbd>Espaço</kbd> transmissão · <kbd>S</kbd> foto · <kbd>F</kbd> tela cheia',
    photo_taken: 'Foto capturada', photo_failed: 'Falha ao capturar a foto', stream_lost: 'Transmissão interrompida',
    settings_failed: 'Não foi possível ler os ajustes da câmera', control_failed: 'Não foi possível ajustar “{0}”',
    sec_image: 'Imagem', sec_exposure: 'Exposição e ganho', sec_wb: 'Balanço de branco', sec_advanced: 'Avançado',
    framesize: 'Resolução', quality: 'Compressão JPEG', brightness: 'Brilho', contrast: 'Contraste', saturation: 'Saturação',
    special_effect: 'Efeito', hmirror: 'Espelhar horizontalmente', vflip: 'Inverter verticalmente',
    aec: 'Exposição automática', aec2: 'Exposição por DSP', ae_level: 'Compensação', aec_value: 'Tempo de exposição',
    agc: 'Ganho automático', agc_gain: 'Ganho', gainceiling: 'Limite de ganho',
    awb: 'Balanço automático', awb_gain: 'Ganho de balanço', wb_mode: 'Iluminação',
    xclk: 'Clock do sensor (XCLK)', lenc: 'Correção de lente', raw_gma: 'Correção de gama', bpc: 'Corrigir pixels pretos',
    wpc: 'Corrigir pixels brancos', dcw: 'Redução por downsize (DCW)', colorbar: 'Barras de teste',
    effects: ['Nenhum', 'Negativo', 'Tons de cinza', 'Avermelhado', 'Esverdeado', 'Azulado', 'Sépia'],
    wb_modes: ['Automático', 'Ensolarado', 'Nublado', 'Escritório', 'Casa'],
  },
  en: {
    locale: 'en-US', language: 'Language',
    connecting: 'Connecting…', camera: 'Camera', sensor: 'Sensor {0}',
    status_connecting: 'Connecting', status_online: 'Online', status_live: 'Live', status_offline: 'Offline',
    open_settings: 'Open settings', close_settings: 'Close settings', settings_title: 'Camera settings',
    stream_alt: 'Camera stream', start_stream: 'Start stream',
    paused_title: 'Stream paused', paused_text: 'Start the live stream or take a photo at any time.',
    waiting_title: 'Connecting…', waiting_text: 'Waiting for the first frame from the camera.',
    lost_title: 'Stream interrupted', lost_text: 'The connection to the camera was lost or the viewer limit was reached.',
    play: 'Start', pause: 'Pause', play_title: 'Start/pause (space)',
    snap: 'Photo', snap_title: 'Take photo (S)', fullscreen_title: 'Fullscreen (F)',
    stat_uptime: 'Uptime', stat_viewers: 'Viewers', stat_frames: 'Frames sent', stat_bytes: 'Data sent',
    stat_signal: 'Wi-Fi signal', stat_memory: 'Free memory', memory_hint: 'Internal RAM · PSRAM',
    signal: ['Excellent', 'Good', 'Fair', 'Weak'],
    shots_title: 'Photos from this session', clear: 'Clear', download: 'Download {0}',
    shortcuts: '<kbd>Space</kbd> stream · <kbd>S</kbd> photo · <kbd>F</kbd> fullscreen',
    photo_taken: 'Photo captured', photo_failed: 'Failed to take the photo', stream_lost: 'Stream interrupted',
    settings_failed: 'Could not read the camera settings', control_failed: 'Could not change “{0}”',
    sec_image: 'Image', sec_exposure: 'Exposure & gain', sec_wb: 'White balance', sec_advanced: 'Advanced',
    framesize: 'Resolution', quality: 'JPEG compression', brightness: 'Brightness', contrast: 'Contrast', saturation: 'Saturation',
    special_effect: 'Effect', hmirror: 'Mirror horizontally', vflip: 'Flip vertically',
    aec: 'Auto exposure', aec2: 'DSP exposure', ae_level: 'Compensation', aec_value: 'Exposure time',
    agc: 'Auto gain', agc_gain: 'Gain', gainceiling: 'Gain ceiling',
    awb: 'Auto white balance', awb_gain: 'White balance gain', wb_mode: 'Lighting',
    xclk: 'Sensor clock (XCLK)', lenc: 'Lens correction', raw_gma: 'Gamma correction', bpc: 'Black pixel correction',
    wpc: 'White pixel correction', dcw: 'Downsize (DCW)', colorbar: 'Test pattern',
    effects: ['None', 'Negative', 'Grayscale', 'Red tint', 'Green tint', 'Blue tint', 'Sepia'],
    wb_modes: ['Auto', 'Sunny', 'Cloudy', 'Office', 'Home'],
  },
};

const storage = {
  get(key) { try { return localStorage.getItem(key); } catch { return null; } },
  set(key, value) { try { localStorage.setItem(key, value); } catch {} },
};
const savedLang = storage.get('camera_server.lang');
let lang = I18N[savedLang] ? savedLang : (navigator.language || '').toLowerCase().startsWith('pt') ? 'pt' : 'en';
const t = (key, ...args) => {
  const value = I18N[lang][key] ?? key;
  return typeof value === 'string' ? value.replace(/\{(\d)\}/g, (_, i) => args[i]) : value;
};

const fileSlug = document.body.dataset.name.toLowerCase().normalize('NFD').replace(/[\u0300-\u036f]/g, '').replace(/[^a-z0-9]+/g, '-').replace(/^-+|-+$/g, '') || 'camera';
const GAIN_CEILING = ['2x', '4x', '8x', '16x', '32x', '64x', '128x'];
const signed = (v) => (v > 0 ? `+${v}` : `${v}`);

const SECTIONS = [
  { title: 'sec_image', icon: 'image', open: true, controls: [
    { id: 'framesize', type: 'select' },
    { id: 'quality', type: 'range', min: 4, max: 63 },
    { id: 'brightness', type: 'range', min: -2, max: 2, format: signed },
    { id: 'contrast', type: 'range', min: -2, max: 2, format: signed },
    { id: 'saturation', type: 'range', min: -2, max: 2, format: signed },
    { id: 'special_effect', type: 'select', options: 'effects' },
    { id: 'hmirror', type: 'toggle' },
    { id: 'vflip', type: 'toggle' },
  ]},
  { title: 'sec_exposure', icon: 'sun', controls: [
    { id: 'aec', type: 'toggle' },
    { id: 'aec2', type: 'toggle', when: (s) => s.aec },
    { id: 'ae_level', type: 'range', min: -2, max: 2, format: signed, when: (s) => s.aec },
    { id: 'aec_value', type: 'range', min: 0, max: 1200, when: (s) => !s.aec },
    { id: 'agc', type: 'toggle' },
    { id: 'agc_gain', type: 'range', min: 0, max: 30, format: (v) => `${v + 1}x`, when: (s) => !s.agc },
    { id: 'gainceiling', type: 'range', min: 0, max: 6, format: (v) => GAIN_CEILING[v], when: (s) => s.agc },
  ]},
  { title: 'sec_wb', icon: 'drop', controls: [
    { id: 'awb', type: 'toggle' },
    { id: 'awb_gain', type: 'toggle', when: (s) => s.awb },
    { id: 'wb_mode', type: 'select', options: 'wb_modes', when: (s) => s.awb && s.awb_gain },
  ]},
  { title: 'sec_advanced', icon: 'cpu', controls: [
    { id: 'xclk', type: 'range', min: 8, max: 24, format: (v) => `${v} MHz` },
    { id: 'lenc', type: 'toggle' },
    { id: 'raw_gma', type: 'toggle' },
    { id: 'bpc', type: 'toggle' },
    { id: 'wpc', type: 'toggle' },
    { id: 'dcw', type: 'toggle' },
    { id: 'colorbar', type: 'toggle' },
  ]},
];
const CONTROLS = Object.fromEntries(SECTIONS.flatMap((s) => s.controls).map((c) => [c.id, c]));

let settings = {};
let system = null;
let streaming = false;
let online = null;
let placeholder = 'paused';

async function request(url, options) {
  const response = await fetch(url, { cache: 'no-store', ...options });
  if (!response.ok) throw new Error((await response.text()) || response.statusText);
  return response;
}
const api = {
  settings: () => request('/api/settings').then((r) => r.json()),
  system: () => request('/api/system').then((r) => r.json()),
  control: (id, value) => request(`/api/control?var=${id}&val=${value}`, { method: 'POST' }).then((r) => r.json()),
  capture: () => request(`/capture?t=${Date.now()}`).then((r) => r.blob()),
};

function toast(message, type = 'ok') {
  const node = document.createElement('div');
  node.className = `toast ${type}`;
  node.innerHTML = `${svg(type === 'error' ? 'alert' : 'check')}<span></span>`;
  node.querySelector('span').textContent = message;
  $('#toasts').append(node);
  setTimeout(() => { node.classList.add('out'); setTimeout(() => node.remove(), 300); }, 2600);
}

function buildControls() {
  const root = $('#controls');
  for (const section of SECTIONS) {
    const details = document.createElement('details');
    details.className = 'section';
    details.open = !!section.open;
    details.innerHTML = `<summary><span class="ico">${svg(section.icon)}</span><span data-i18n="${section.title}"></span><span class="chev">${svg('chevron')}</span></summary><div class="section-body"></div>`;
    const body = details.querySelector('.section-body');
    for (const control of section.controls) body.append(buildRow(control));
    root.append(details);
  }
}

function buildRow(control) {
  const row = document.createElement('div');
  const id = `ctl-${control.id}`;
  const label = `<label for="${id}" data-i18n="${control.id}"></label>`;
  row.className = control.type === 'toggle' ? 'row inline' : 'row';
  if (control.type === 'toggle') {
    row.innerHTML = `${label}<span class="switch"><input type="checkbox" role="switch" id="${id}"><i></i></span>`;
    row.querySelector('input').addEventListener('change', (e) => send(control, e.target.checked ? 1 : 0));
  } else if (control.type === 'range') {
    row.innerHTML = `<div class="row-head">${label}<output></output></div>
      <input type="range" id="${id}" min="${control.min}" max="${control.max}" step="1">`;
    const input = row.querySelector('input');
    input.addEventListener('input', () => paintRange(control, input));
    input.addEventListener('change', () => send(control, Number(input.value)));
  } else {
    row.innerHTML = `<div class="row-head">${label}</div><select id="${id}"></select>`;
    row.querySelector('select').addEventListener('change', (e) => send(control, Number(e.target.value)));
  }
  control.row = row;
  control.input = row.querySelector('input,select');
  return row;
}

function fillOptions(control, options) {
  const select = control.input;
  const value = select.value;
  select.replaceChildren(...options.map(({ value, label }) => new Option(label, value)));
  if (value !== '') select.value = value;
}

function fillTranslatedOptions() {
  for (const control of Object.values(CONTROLS)) {
    if (control.options) fillOptions(control, t(control.options).map((label, value) => ({ value, label })));
  }
}

function paintRange(control, input) {
  const value = Number(input.value);
  input.style.setProperty('--p', `${((value - control.min) / (control.max - control.min)) * 100}%`);
  control.row.querySelector('output').textContent = control.format ? control.format(value) : value;
}

const resolutionOf = (id) => (settings.framesizes || []).find((f) => f.id === id);
const resolutionLabel = (id) => { const r = resolutionOf(id); return r ? `${r.w}×${r.h}` : '—'; };

function applySettings(next) {
  settings = next;
  if (next.framesizes && CONTROLS.framesize.input.options.length !== next.framesizes.length) {
    fillOptions(CONTROLS.framesize, [...next.framesizes].reverse().map((f) => ({ value: f.id, label: `${f.w} × ${f.h}` })));
  }
  for (const control of Object.values(CONTROLS)) {
    const value = next[control.id];
    if (value === undefined) { control.row.hidden = true; continue; }
    control.row.hidden = false;
    if (control.type === 'toggle') control.input.checked = !!value;
    else control.input.value = value;
    if (control.type === 'range') paintRange(control, control.input);
    control.row.classList.toggle('disabled', control.when ? !control.when(next) : false);
  }
  const resolution = resolutionLabel(next.framesize);
  $('#res-pill').textContent = resolution;
  $('#badge-res').textContent = resolution;
  $('#subtitle').textContent = next.sensor ? t('sensor', next.sensor) : t(online === false ? 'camera' : 'connecting');
}

async function send(control, value) {
  control.row.classList.add('busy');
  try {
    applySettings(await api.control(control.id, value));
  } catch {
    toast(t('control_failed', t(control.id)), 'error');
    applySettings(settings);
  } finally {
    control.row.classList.remove('busy');
  }
}

const viewer = $('#viewer');
const img = $('#stream');

function renderPlaceholder() {
  const loading = placeholder === 'waiting';
  $('#ph-title').textContent = t(`${placeholder}_title`);
  $('#ph-text').textContent = t(`${placeholder}_text`);
  $('#play-big').innerHTML = loading ? '' : svg('play');
  $('#play-big').className = loading ? 'spinner' : 'play-big';
}

function updatePlayButton() {
  $('#play-icon').innerHTML = ICONS[streaming ? 'pause' : 'play'];
  $('#play-label').textContent = t(streaming ? 'pause' : 'play');
  updateStatus();
}

function startStream() {
  streaming = true;
  placeholder = 'waiting';
  renderPlaceholder();
  img.src = `/stream?t=${Date.now()}`;
  updatePlayButton();
}

function stopStream(reason = 'paused') {
  streaming = false;
  img.src = 'data:,';
  viewer.classList.remove('has-frame');
  placeholder = reason;
  renderPlaceholder();
  updatePlayButton();
}

img.addEventListener('load', () => { if (streaming) viewer.classList.add('has-frame'); });
img.addEventListener('error', () => {
  if (!streaming) return;
  stopStream('lost');
  toast(t('stream_lost'), 'error');
});

const toggleStream = () => (streaming ? stopStream() : startStream());

async function snapshot() {
  const button = $('#snap');
  button.disabled = true;
  try {
    const blob = await api.capture();
    const flash = $('#flash');
    flash.classList.remove('go');
    void flash.offsetWidth;
    flash.classList.add('go');
    addShot(blob);
    toast(t('photo_taken'));
  } catch {
    toast(t('photo_failed'), 'error');
  } finally {
    button.disabled = false;
  }
}

function addShot(blob) {
  const now = new Date();
  const url = URL.createObjectURL(blob);
  const pad = (n) => String(n).padStart(2, '0');
  const name = `${fileSlug}_${now.getFullYear()}${pad(now.getMonth() + 1)}${pad(now.getDate())}_${pad(now.getHours())}${pad(now.getMinutes())}${pad(now.getSeconds())}.jpg`;
  const link = document.createElement('a');
  link.className = 'shot';
  link.href = url;
  link.download = name;
  link.dataset.taken = now.getTime();
  link.innerHTML = `<img alt=""><span><time></time>${svg('download')}</span>`;
  link.querySelector('img').src = url;
  const gallery = $('#gallery');
  gallery.prepend(link);
  while (gallery.children.length > 12) {
    URL.revokeObjectURL(gallery.lastElementChild.href);
    gallery.lastElementChild.remove();
  }
  localizeShots();
  $('#shots').hidden = false;
}

function localizeShots() {
  for (const link of $('#gallery').children) {
    link.title = t('download', link.download);
    link.querySelector('time').textContent = new Date(Number(link.dataset.taken)).toLocaleTimeString(t('locale'));
  }
}

function clearShots() {
  for (const link of $('#gallery').children) URL.revokeObjectURL(link.href);
  $('#gallery').replaceChildren();
  $('#shots').hidden = true;
}

function toggleFullscreen() {
  if (document.fullscreenElement) document.exitFullscreen();
  else viewer.requestFullscreen?.();
}

function formatDuration(seconds) {
  const d = Math.floor(seconds / 86400), h = Math.floor((seconds % 86400) / 3600), m = Math.floor((seconds % 3600) / 60), s = seconds % 60;
  if (d) return `${d}d ${h}h`;
  if (h) return `${h}h ${m}min`;
  return m ? `${m}min ${s}s` : `${s}s`;
}
function formatBytes(bytes) {
  const units = ['B', 'KB', 'MB', 'GB', 'TB'];
  let i = 0;
  while (bytes >= 1024 && i < units.length - 1) { bytes /= 1024; i++; }
  return `${bytes.toLocaleString(t('locale'), { maximumFractionDigits: i && bytes < 100 ? 1 : 0 })} ${units[i]}`;
}
function signalLabel(rssi) {
  if (!rssi) return '—';
  const quality = t('signal')[rssi >= -55 ? 0 : rssi >= -67 ? 1 : rssi >= -75 ? 2 : 3];
  return `${quality} · ${rssi} dBm`;
}

function updateStatus() {
  const pill = $('#status');
  const state = online === false ? 'offline' : streaming && viewer.classList.contains('has-frame') ? 'live' : online ? 'online' : 'connecting';
  pill.className = `pill ${state}`;
  pill.querySelector('span').textContent = t(`status_${state}`);
}

function renderSystem() {
  if (!system) return;
  const s = system;
  const fps = `${s.fps} fps`;
  $('#fps-pill').textContent = fps;
  $('#badge-fps').textContent = fps;
  $('#st-uptime').textContent = formatDuration(s.uptime);
  $('#st-streams').textContent = `${s.streams} / ${s.max_streams}`;
  $('#st-frames').textContent = s.frames.toLocaleString(t('locale'));
  $('#st-bytes').textContent = formatBytes(s.bytes);
  $('#st-rssi').textContent = signalLabel(s.rssi);
  $('#st-heap').textContent = s.psram_total ? `${formatBytes(s.heap_free)} · ${formatBytes(s.psram_free)}` : formatBytes(s.heap_free);
}

async function poll() {
  try {
    system = await api.system();
    if (online === false) api.settings().then(applySettings).catch(() => {});
    online = true;
    renderSystem();
  } catch {
    online = false;
  }
  updateStatus();
  setTimeout(poll, 2000);
}

function setLanguage(next) {
  lang = next;
  storage.set('camera_server.lang', next);
  document.documentElement.lang = t('locale');
  for (const node of document.querySelectorAll('[data-i18n]')) node.textContent = t(node.dataset.i18n);
  for (const node of document.querySelectorAll('[data-i18n-html]')) node.innerHTML = t(node.dataset.i18nHtml);
  for (const node of document.querySelectorAll('[data-i18n-title]')) node.title = t(node.dataset.i18nTitle);
  for (const node of document.querySelectorAll('[data-i18n-aria]')) node.setAttribute('aria-label', t(node.dataset.i18nAria));
  for (const node of document.querySelectorAll('[data-i18n-alt]')) node.alt = t(node.dataset.i18nAlt);
  for (const button of document.querySelectorAll('[data-lang]')) button.setAttribute('aria-pressed', button.dataset.lang === lang);
  fillTranslatedOptions();
  applySettings(settings);
  renderPlaceholder();
  updatePlayButton();
  renderSystem();
  localizeShots();
}

function setPanel(open) {
  $('#panel').classList.toggle('open', open);
  $('#scrim').classList.toggle('open', open);
}

$('#play').addEventListener('click', toggleStream);
$('#play-big').addEventListener('click', () => { if (!streaming) startStream(); });
$('#snap').addEventListener('click', snapshot);
$('#fullscreen').addEventListener('click', toggleFullscreen);
$('#clear-shots').addEventListener('click', clearShots);
$('#panel-toggle').addEventListener('click', () => setPanel(true));
$('#panel-close').addEventListener('click', () => setPanel(false));
$('#scrim').addEventListener('click', () => setPanel(false));
for (const button of document.querySelectorAll('[data-lang]')) button.addEventListener('click', () => setLanguage(button.dataset.lang));
document.addEventListener('keydown', (e) => {
  if (e.target.closest('input,select,textarea') || e.metaKey || e.ctrlKey || e.altKey) return;
  const actions = { ' ': toggleStream, s: snapshot, f: toggleFullscreen };
  const action = actions[e.key.toLowerCase()];
  if (action) { e.preventDefault(); action(); }
});
setInterval(updateStatus, 1000);

buildControls();
setLanguage(lang);
api.settings().then(applySettings).catch(() => toast(t('settings_failed'), 'error'));
poll();
startStream();
</script>
</body>
</html>
)html";

}  // namespace camera_server
