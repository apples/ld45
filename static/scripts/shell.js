var loadingScreenElement = document.getElementById('loading-screen');
var statusElement = document.getElementById('status');
var spinnerElement = document.getElementById('spinner');

(function () {
    var lastTarget;
    var body = document.querySelector('body');
    var canvas = document.getElementById('canvas');

    var activateCanvas = function (event) {
        lastTarget = event.target;
        window.focus();
    };

    var preventScroll = function (event) {
        var keyCodes = [32, 37, 38, 39, 40];
        if (lastTarget != canvas) {
            return false;
        }
        if (keyCodes.includes(event.keyCode)) {
            event.preventDefault();
        }
    };

    var handleMouseDown = function (event) {
        window.focus();
        event.preventDefault();
        event.stopPropagation();
        event.target.style.cursor = 'default';
    };

    body.addEventListener('keydown', preventScroll, false);
    body.addEventListener('click', activateCanvas, false);
    canvas.addEventListener('mousedown', handleMouseDown, false);
})();

var Module = {
    preRun: [],
    postRun: [],
    print: (function () {
        var element = document.getElementById('output');
        if (element) element.value = ''; // clear browser cache
        return function (text) {
            if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
            console.log(text);
        };
    })(),
    printErr: function (text) {
        if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
        console.error(text);
    },
    canvas: (function () {
        var canvas = document.getElementById('canvas');

        // As a default initial behavior, pop up an alert when webgl context is lost. To make your
        // application robust, you may want to override this behavior before shipping!
        // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
        canvas.addEventListener("webglcontextlost", function (e) { alert('WebGL context lost. You will need to reload the page.'); e.preventDefault(); }, false);

        return canvas;
    })(),
    scaleCanvas: function () {
        if ('running' in Module.canvas.dataset) {
            var r = 800/600;
            var w = window.innerWidth;
            var h = window.innerHeight;
            if (w/h > r) {
                w = h * r;
            } else {
                h = w / r;
            }
            Module.canvas.style.width = w + 'px';
            Module.canvas.style.height = h + 'px';
        } else {
            Module.canvas.style.width = null;
            Module.canvas.style.height = null;
        }
    },
    setStatus: function (text) {
        if (!text) {
            loadingScreenElement.hidden = true;
            Module.canvas.dataset.running = "";
        } else {
            if (text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/)) {
                spinnerElement.hidden = false;
            } else {
                spinnerElement.hidden = true;
            }
            statusElement.innerHTML = text;
            loadingScreenElement.hidden = false;
            delete Module.canvas.dataset.running;
        }
        Module.scaleCanvas();
    },
    totalDependencies: 0,
    monitorRunDependencies: function (left) {
        this.totalDependencies = Math.max(this.totalDependencies, left);
        Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies - left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
    }
};

window.addEventListener('resize', function (e) {
    Module.scaleCanvas();
});

window.onerror = function (message, url, line, column, e) {
    Module.setStatus('Exception thrown, see JavaScript console');
    spinnerElement.style.display = 'none';
    Module.setStatus = function (text) {
        if (text) Module.printErr('[post-exception status] ' + text);
    };
    if (typeof e === 'number') {
        var pointer = e;
        Module.ccall('what_to_stderr', 'number', ['number'], [pointer]);
    }
};

Module.setStatus('Downloading...');
