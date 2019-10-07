
var LibraryEmberWebsocket = {
    $EmberWebsocket: {
        websockets: [],
        websocket_next: 1,
        add: function(ws) {
            this.websockets[this.websocket_next] = ws;
            return this.websocket_next++;
        },
        get: function(handle) {
            return this.websockets[handle];
        },
        remove: function(handle) {
            delete this.websockets[handle];
        },
    },
    ember_ws_create: function(addr) {
        var addr_str = Module.Pointer_stringify(addr);
        var handle = new WebSocket("wss://" + addr_str + "/");
        return EmberWebsocket.add(handle);
    },
    ember_ws_destroy: function(handle) {
        var ws = EmberWebsocket.get(handle);
        if (ws) {
            ws.close();
            EmberWebsocket.remove(handle);
            return 1;
        } else {
            return 0;
        }
    },
    ember_ws_onopen: function(handle, callback, callback_data) {
        var ws = EmberWebsocket.get(handle);
        if (ws) {
            if (callback !== 0) {
                ws.onopen = function() {
                    Runtime.dynCall('vi', callback, [callback_data]);
                };
            } else {
                ws.onopen = null;
            }
            return 1;
        } else {
            return 0;
        }
    },
    ember_ws_onmessage: function(handle, callback, callback_data) {
        var ws = EmberWebsocket.get(handle);
        if (ws) {
            if (callback !== 0) {
                ws.onmessage = function(msg) {
                    var str = msg.data;
                    var len = lengthBytesUTF8(str) + 1;
                    var mem = _malloc(len);
                    stringToUTF8(str, mem, len);
                    try {
                        Runtime.dynCall('vii', callback, [callback_data, mem]);
                    } finally {
                        _free(mem);
                    }
                };
            } else {
                ws.onmessage = null;
            }
            return 1;
        } else {
            return 0;
        }
    },
    ember_ws_onclose: function(handle, callback, callback_data) {
        var ws = EmberWebsocket.get(handle);
        if (ws) {
            if (callback !== 0) {
                ws.onclose = function(msg) {
                    var code = msg.code;
                    var reason = msg.reason;
                    var reason_len = lengthBytesUTF8(reason) + 1;
                    var reason_mem = _malloc(reason_len);
                    stringToUTF8(reason, reason_mem, reason_len);
                    try {
                        Runtime.dynCall('viii', callback, [callback_data, code, reason_mem]);
                    } finally {
                        _free(reason_mem);
                    }
                };
            } else {
                ws.onclose = null;
            }
            return 1;
        } else {
            return 0;
        }
    },
    ember_ws_send: function(handle, msg) {
        var msg_str = Module.Pointer_stringify(msg);
        var ws = EmberWebsocket.get(handle);
        if (ws && ws.readyState === 1) {
            ws.send(msg_str);
            return 1;
        } else {
            return 0;
        }
    },
    ember_ws_get_state: function(handle) {
        var ws = EmberWebsocket.get(handle);
        if (ws) {
            return ws.readyState;
        } else {
            return -1;
        }
    },
};

autoAddDeps(LibraryEmberWebsocket, '$EmberWebsocket');
mergeInto(LibraryManager.library, LibraryEmberWebsocket);
