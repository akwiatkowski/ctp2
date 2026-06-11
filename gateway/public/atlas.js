// The marginalia card: one floating tooltip for the whole chart.
// Event delegation on the grid — 4000+ tiles, zero per-tile listeners.
(function () {
  "use strict";
  var grid = document.getElementById("atlas");
  var tip = document.getElementById("atlas-tip");
  if (!grid || !tip) return;

  function esc(s) {
    return s.replace(/[&<>"]/g, function (c) {
      return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c];
    });
  }

  grid.addEventListener("mouseover", function (e) {
    var t = e.target.closest(".tl");
    if (!t || !t.dataset.t) { tip.hidden = true; return; }
    var d = t.dataset;
    var html = '<div class="tip__pos">(' + esc(d.x) + ", " + esc(d.y) + ")</div>";
    html += "<div>" + esc(d.t) + "</div>";
    if (d.city) html += '<div class="tip__city">⊙ ' + esc(d.city) + "</div>";
    if (d.unit) html += '<div class="tip__unit">' + esc(d.unit) + "</div>";
    if (d.rank) html += '<div class="tip__city">survey site #' + esc(d.rank) + "</div>";
    tip.innerHTML = html;
    tip.hidden = false;
  });

  grid.addEventListener("mousemove", function (e) {
    if (tip.hidden) return;
    var pad = 14;
    var x = e.clientX + pad, y = e.clientY + pad;
    var r = tip.getBoundingClientRect();
    if (x + r.width > window.innerWidth - 8) x = e.clientX - r.width - pad;
    if (y + r.height > window.innerHeight - 8) y = e.clientY - r.height - pad;
    tip.style.left = x + "px";
    tip.style.top = y + "px";
  });

  grid.addEventListener("mouseleave", function () { tip.hidden = true; });
})();
