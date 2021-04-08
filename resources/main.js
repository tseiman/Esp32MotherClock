
console.log("Hello");

var ws = new WebSocket("wss://" + location.host + "/ws");

ws.onerror = function(event) {
  console.error("WebSocket error observed:", event);
};

ws.onmessage = function (event) {
  console.log(event.data);
}

function wsSend(data) {
    ws.send(data);
}