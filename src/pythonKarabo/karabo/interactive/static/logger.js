'use strict';

window.onload = start;

var updater = {
    node: null,
    socket: null,
    server_name: null,

    start: function() {
        updater.node = document.getElementsByClassName("deamonlog")[0]
        updater.server_name = updater.node.id;
        var url = "ws://" + location.host + "/api/servers/logsocket";
        updater.socket = new WebSocket(url);
        updater.socket.onopen = updater.open.bind(updater);
        updater.socket.onmessage = updater.process.bind(updater);
    },

    open: function() {
        var s = JSON.stringify({"type": "log",
                                "server": this.server_name});
        console.log(s);
        this.socket.send(s);
        console.log('SENT!');
    },

    process: function(event) {
        var row = JSON.parse(event.data);
        var existing = document.getElementById("r" + row.id);
        if (!!existing) return;
        var div = document.createElement("div");
        div.textContent = row.text;
        div.setAttribute('class', 'row');
        div.setAttribute('id', "r" + row.id);
        this.node.appendChild(div);
    },

    request: function(req) {
        if (this.socket.readyState == WebSocket.OPEN) {
            this.socket.send(req);
        }
    },
};

function start() {
    updater.start();
}
