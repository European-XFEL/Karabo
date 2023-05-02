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

    startstop: function() {
        // Close the connection, if open.
        var btn = document.getElementById("livecontrolplay")
        if (updater.socket.readyState === WebSocket.OPEN) {
            btn.value = "Restart";
            updater.socket.close();
        } else {
            window.location.reload();
        }
    },

    open: function() {
        var s = JSON.stringify({"type": "log",
                                "server": this.server_name});
        this.socket.send(s);
    },

    process: function(event) {
        if (event.data == "RTS") {
            this.socket.send("CTS");
            window.scrollTo(0,document.body.scrollHeight);
            return;
        }
        var row = JSON.parse(event.data);
        var div = document.createElement("div");
        div.textContent = row.text;
        div.setAttribute('class', 'row');
        this.node.appendChild(div);
    },
};

function start() {
    updater.start();
}
