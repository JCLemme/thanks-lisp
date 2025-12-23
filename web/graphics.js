// Graphics system for WASM Lisp interpreter
// Provides virtual /tmp/gfx_pipe file that renders commands to HTML canvas

export const GRAPHICS_CONFIG = {
  SCALE_FACTOR: 2,              // CSS pixel multiplier
  CANVAS_WIDTH: 240,            // Logical pixels
  CANVAS_HEIGHT: 240,           // Logical pixels
  DEFAULT_COLOR: [0, 0, 0],     // Black
  BACKGROUND_COLOR: [255, 255, 255], // White
  COMMAND_BUFFER_MAX: 4096,     // Bytes
  DEBUG: false,                  // Console logging
  FONT_FAMILY: 'LispM, monospace',
  FONT_SIZE: '12px'
};

export class GraphicsRenderer {
  constructor(canvasElement, config = GRAPHICS_CONFIG) {
    this.canvas = canvasElement;
    this.config = config;

    // Set canvas logical size
    canvasElement.width = config.CANVAS_WIDTH;
    canvasElement.height = config.CANVAS_HEIGHT;

    // Set CSS display size (scaled)
    const displayWidth = config.CANVAS_WIDTH * config.SCALE_FACTOR;
    const displayHeight = config.CANVAS_HEIGHT * config.SCALE_FACTOR;
    canvasElement.style.width = `${displayWidth}px`;
    canvasElement.style.height = `${displayHeight}px`;

    // Get context
    this.ctx = canvasElement.getContext('2d');

    // Initialize state
    this.drawColor = {
      r: config.DEFAULT_COLOR[0],
      g: config.DEFAULT_COLOR[1],
      b: config.DEFAULT_COLOR[2]
    };
    this.commandBuffer = '';

    // Clear to black
    this.ctx.fillStyle = `rgb(${config.BACKGROUND_COLOR.join(',')})`;
    this.ctx.fillRect(0, 0, config.CANVAS_WIDTH, config.CANVAS_HEIGHT);
  }

  processBytes(bytes) {
    // Convert bytes to UTF-8 string
    const text = new TextDecoder('utf-8').decode(bytes);
    this.commandBuffer += text;

    // Split on newlines
    let lines = this.commandBuffer.split('\n');

    // Keep last incomplete line in buffer
    this.commandBuffer = lines.pop() || '';

    // Limit buffer size to prevent memory issues
    if (this.commandBuffer.length > this.config.COMMAND_BUFFER_MAX) {
      console.error('[GFX] Command buffer overflow, clearing');
      this.commandBuffer = '';
    }

    // Execute each complete command
    for (let line of lines) {
      const trimmed = line.trim();
      if (trimmed.length > 0) {
        this.executeCommand(trimmed);
      }
    }
  }

  executeCommand(cmdLine) {
    // Split on whitespace
    const parts = cmdLine.split(/\s+/);
    const cmd = parts[0].toLowerCase();

    if (this.config.DEBUG) {
      console.log('[GFX]', cmdLine);
    }

    try {
      switch (cmd) {
        case 'clear':
          this.cmdClear();
          break;

        case 'color':
          this.cmdColor(
            parseInt(parts[1]),
            parseInt(parts[2]),
            parseInt(parts[3])
          );
          break;

        case 'point':
          this.cmdPoint(
            parseInt(parts[1]),
            parseInt(parts[2])
          );
          break;

        case 'line':
          this.cmdLine(
            parseInt(parts[1]), parseInt(parts[2]),
            parseInt(parts[3]), parseInt(parts[4])
          );
          break;

        case 'rect':
          this.cmdRect(
            parseInt(parts[1]), parseInt(parts[2]),
            parseInt(parts[3]), parseInt(parts[4])
          );
          break;

        case 'fillrect':
          this.cmdFillRect(
            parseInt(parts[1]), parseInt(parts[2]),
            parseInt(parts[3]), parseInt(parts[4])
          );
          break;

        case 'circle':
          this.cmdCircle(
            parseInt(parts[1]),
            parseInt(parts[2]),
            parseInt(parts[3])
          );
          break;

        case 'fillcircle':
          this.cmdFillCircle(
            parseInt(parts[1]),
            parseInt(parts[2]),
            parseInt(parts[3])
          );
          break;

        case 'text':
          const x = parseInt(parts[1]);
          const y = parseInt(parts[2]);
          const message = parts.slice(3).join(' ');
          this.cmdText(x, y, message);
          break;

        default:
          console.error('[GFX] Unknown command:', cmd);
      }
    } catch (err) {
      console.error('[GFX] Command error:', cmdLine, err);
    }
  }

  getColorString() {
    return `rgb(${this.drawColor.r}, ${this.drawColor.g}, ${this.drawColor.b})`;
  }

  cmdClear() {
    this.ctx.fillStyle = `rgb(${this.config.BACKGROUND_COLOR.join(',')})`;
    this.ctx.fillRect(0, 0, this.config.CANVAS_WIDTH, this.config.CANVAS_HEIGHT);
  }

  cmdColor(r, g, b) {
    if (isNaN(r) || isNaN(g) || isNaN(b)) {
      console.error('[GFX] Invalid color values');
      return;
    }
    this.drawColor = {
      r: Math.max(0, Math.min(255, r)),
      g: Math.max(0, Math.min(255, g)),
      b: Math.max(0, Math.min(255, b))
    };
  }

  cmdPoint(x, y) {
    if (isNaN(x) || isNaN(y)) return;
    this.ctx.fillStyle = this.getColorString();
    this.ctx.fillRect(x, y, 1, 1);
  }

  cmdLine(x1, y1, x2, y2) {
    if (isNaN(x1) || isNaN(y1) || isNaN(x2) || isNaN(y2)) return;
    this.ctx.strokeStyle = this.getColorString();
    this.ctx.lineWidth = 1;
    this.ctx.beginPath();
    this.ctx.moveTo(x1 + 0.5, y1 + 0.5);
    this.ctx.lineTo(x2 + 0.5, y2 + 0.5);
    this.ctx.stroke();
  }

  cmdRect(x, y, w, h) {
    if (isNaN(x) || isNaN(y) || isNaN(w) || isNaN(h)) return;
    this.ctx.strokeStyle = this.getColorString();
    this.ctx.lineWidth = 1;
    this.ctx.strokeRect(x + 0.5, y + 0.5, w, h);
  }

  cmdFillRect(x, y, w, h) {
    if (isNaN(x) || isNaN(y) || isNaN(w) || isNaN(h)) return;
    this.ctx.fillStyle = this.getColorString();
    this.ctx.fillRect(x, y, w, h);
  }

  cmdCircle(cx, cy, radius) {
    if (isNaN(cx) || isNaN(cy) || isNaN(radius)) return;
    this.drawMidpointCircle(cx, cy, radius, false);
  }

  cmdFillCircle(cx, cy, radius) {
    if (isNaN(cx) || isNaN(cy) || isNaN(radius)) return;
    this.ctx.fillStyle = this.getColorString();
    this.ctx.beginPath();
    this.ctx.arc(cx, cy, radius, 0, 2 * Math.PI);
    this.ctx.fill();
  }

  cmdText(x, y, message) {
    if (isNaN(x) || isNaN(y) || !message) return;
    this.ctx.fillStyle = this.getColorString();
    this.ctx.font = `${this.config.FONT_SIZE} ${this.config.FONT_FAMILY}`;
    this.ctx.textBaseline = 'top';
    this.ctx.fillText(message, x, y);
  }

  drawMidpointCircle(cx, cy, radius, fill) {
    if (fill) {
      this.ctx.fillStyle = this.getColorString();
      this.ctx.beginPath();
      this.ctx.arc(cx, cy, radius, 0, 2 * Math.PI);
      this.ctx.fill();
      return;
    }

    // Outline circles: use midpoint algorithm for exact match with native
    this.ctx.fillStyle = this.getColorString();

    let x = radius;
    let y = 0;
    let err = 0;

    while (x >= y) {
      // Draw 8 symmetric points
      this.ctx.fillRect(cx + x, cy + y, 1, 1);
      this.ctx.fillRect(cx + y, cy + x, 1, 1);
      this.ctx.fillRect(cx - y, cy + x, 1, 1);
      this.ctx.fillRect(cx - x, cy + y, 1, 1);
      this.ctx.fillRect(cx - x, cy - y, 1, 1);
      this.ctx.fillRect(cx - y, cy - x, 1, 1);
      this.ctx.fillRect(cx + y, cy - x, 1, 1);
      this.ctx.fillRect(cx + x, cy - y, 1, 1);

      if (err <= 0) {
        y += 1;
        err += 2 * y + 1;
      }

      if (err > 0) {
        x -= 1;
        err -= 2 * x + 1;
      }
    }
  }
}

export function setupVirtualPipe(Module, graphicsRenderer) {
  // Create /tmp directory if it doesn't exist
  try {
    Module.FS.mkdir('/tmp');
  } catch (e) {
    if (e.code !== 'EEXIST') throw e;
  }

  // Create a custom device with stream operations
  const gfxDevice = Module.FS.makedev(64, 0);

  Module.FS.registerDevice(gfxDevice, {
    open: function(stream) {
      console.log('[GFX] Pipe opened');
    },
    close: function(stream) {
      console.log('[GFX] Pipe closed');
    },
    read: function(stream, buffer, offset, length, pos) {
      // Graphics pipe is write-only
      return 0;
    },
    write: function(stream, buffer, offset, length, pos) {
      //console.log('[GFX] Write:', length, 'bytes');

      // Extract bytes
      const bytes = buffer.subarray(offset, offset + length);
      //console.log('[GFX] Data:', new TextDecoder().decode(bytes));

      // Pass to renderer
      graphicsRenderer.processBytes(bytes);

      // Return number of bytes written
      return length;
    }
  });

  // Create the device node at /tmp/gfx_pipe
  Module.FS.mkdev('/tmp/gfx_pipe', gfxDevice);

  console.log('[GFX] Virtual pipe ready at /tmp/gfx_pipe');
}
