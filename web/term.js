import "https://unpkg.com/@xterm/xterm/lib/xterm.js";
import "https://unpkg.com/@xterm/addon-fit/lib/addon-fit.js"
import { openpty } from "./index.mjs";
import initEmscripten from "./thanks.mjs";
import { GraphicsRenderer, setupVirtualPipe } from "./graphics.js";

var xterm = new Terminal({ fontFamily: 'LispM, monospaced', fontSize: 13, cursorBlink: true, theme: {foreground: "black", background: "white", cursor: "black", selectionForeground: "white", selectionBackground: "black"} });
xterm.open(document.getElementById("terminal"));

const fitAddon = new FitAddon.FitAddon();
xterm.loadAddon(fitAddon);
window.fitAddon = fitAddon;

// Wait for terminal viewport to be fully initialized
requestAnimationFrame(() => {
    requestAnimationFrame(() => {
        fitAddon.fit();
    });
});

// Refit terminal when window is resized
window.addEventListener('resize', () => {
    fitAddon.fit();
});

const { master, slave } = openpty();

xterm.loadAddon(master);

// Initialize graphics BEFORE Emscripten
const canvas = document.getElementById('gfx-canvas');
const gfxRenderer = new GraphicsRenderer(canvas);

// Initialize Emscripten with PTY
window.Module = await initEmscripten({ pty: slave });
// Setup virtual pipe AFTER Module is ready
setupVirtualPipe(window.Module, gfxRenderer);

xterm.attachCustomKeyEventHandler(ev => {
    if(ev.type == 'keydown' && ev.key == "c" && ev.ctrlKey) {
        Module._inthandler(0);
        return false;
    }
});

var loadedGfx = false;
var loadedCurve = false;

function doPlot() {
    if(!loadedCurve) {
        loadedCurve = true;
        master.ldisc.writeFromLower("(load \"examples/curve.lisp\")\n");
    }

    master.ldisc.writeFromLower("\nplot\n");
    master.ldisc.writeFromLower("\n(plot sinfun)\n");
}
window.doPlot = doPlot

function doClock() {
    if(!loadedGfx) {
        loadedGfx = true;
        master.ldisc.writeFromLower("(load \"examples/gfxlib.lisp\")\n");
    }

    master.ldisc.writeFromLower("\nclock-at\n");
    master.ldisc.writeFromLower("\n() ; Press ctrl-c to get back to the REPL...\n");
    master.ldisc.writeFromLower("\n(run-clock)\n");
}
window.doClock = doClock
