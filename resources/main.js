console.log("Hello");

var ws = new WebSocket("wss://" + location.host + "/ws");

ws.onerror = function(event) {
	console.error("WebSocket error observed:", event);
};

ws.onmessage = function (event) {
	var msgObj = JSON.parse(event.data);
	console.log(msgObj);
	if(msgObj.cmd.normalize() === "PING" ) {
		var answer = { "cmd": "PONG"};
		wsSend(JSON.stringify(answer));
	}
}

ws.onopen = function (event) {
		var status = { "cmd": "GETSTATUS"};
    wsSend(JSON.stringify(status));
};

function wsSend(data) {
	ws.send(data);
}

/*	

$( document ).ready(function() {

});
$(window).bind('beforeunload', function(){
ws.send("close!");
ws.close();
});
*/