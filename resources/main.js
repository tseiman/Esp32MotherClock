/* ***************************************************************************
 *
 * Thomas Schmidt, 2021
 *
 * This file is part of the Esp32MotherClock Project
 *
 * This is WebClient, mainly GUI related code
 * 
 * License: MPL-2.0 License
 *
 * Project URL: https://github.com/tseiman/Esp32MotherClock
 *
 ************************************************************************** */


console.log("Esp32MotherClock - OK");

var auth = false;

var getMapDataObj = {};


(function ($) {
    $.fn.shake = function (options) {
        // defaults
        var settings = {
            'shakes': 2,
            'distance': 10,
            'duration': 400
        };
        // merge options
        if (options) {
            $.extend(settings, options);
        }
        // make it so
        var pos;
        return this.each(function () {
            $this = $(this);
            // position if necessary
            pos = $this.css('position');
            if (!pos || pos === 'static') {
                $this.css('position', 'relative');
            }
            // shake it
            for (var x = 1; x <= settings.shakes; x++) {
                $this.animate({ left: settings.distance * -1 }, (settings.duration / settings.shakes) / 4)
                    .animate({ left: settings.distance }, (settings.duration / settings.shakes) / 2)
                    .animate({ left: 0 }, (settings.duration / settings.shakes) / 4);
            }
        });
    };
}(jQuery));

function login(msgObj){
	if(msgObj.auth && (! auth)) {
		$("#mainform").show();
		$("#loginform").hide();
		auth = true;
		var map = { "cmd": "GETMAP"};
    	wsSend(JSON.stringify(map));

	} else if ((! msgObj.auth) && auth) {
		$("#mainform").hide();
		$("#loginform").show();	
		$('input[name="username"]').val('');
		$('input[name="password"]').val('');		
		auth = false;
	} else if ((! msgObj.auth) && (! auth) && msgObj.cmd === "LOGIN") {
		$("#loginform").shake();
	}
} 


function getmap(msgObj){

	if(msgObj.packetNo === 0) { 
		getMapDataObj.receivedBytes = msgObj.len;
		getMapDataObj.buffer = msgObj.data;
	} else {
		getMapDataObj.receivedBytes += msgObj.len;
		getMapDataObj.buffer = getMapDataObj.concat(msgObj.data);
	}

	if(msgObj.totallen === getMapDataObj.receivedBytes) {
		getMapDataObj.buffer = getMapDataObj.buffer.replace(/'/g,'"');
		getMapDataObj.buffer = getMapDataObj.buffer.replace(/\\/g,'\\\\');
		var mapObj = JSON.parse(getMapDataObj.buffer);
		console.log(mapObj);
	}

} 




var ws = new WebSocket("wss://" + location.host + "/ws");

ws.onerror = function(event) {
	console.error("WebSocket error observed:", event);
	$("#spinnerbg").show();
};

ws.onmessage = function (event) {
	
	try {
		var msgObj = JSON.parse(event.data);
		console.log(msgObj);
		if(msgObj.cmd === "PING" ) {
			var answer = { "cmd": "PONG"};
			wsSend(JSON.stringify(answer));
		}

		if(msgObj.cmd === "STATUS" || msgObj.cmd === "LOGIN") {
			login(msgObj);
		}


		if(msgObj.cmd === "GETMAP" ) {
			getmap(msgObj);
		}


	} catch(e) {
		console.log("Error", e, event.data);
	}

}

ws.onopen = function (event) {
	var status = { "cmd": "GETSTATUS"};
    wsSend(JSON.stringify(status));
    $("#spinnerbg").hide();
};

function wsSend(data) {
	ws.send(data);
}


$( document ).ready(function() {
	$( "#loginform-form" ).submit(function( event ) {
		event.preventDefault();
		var username = $( "#loginform-form" ).find('input[name="username"]').val();
		var password = $( "#loginform-form" ).find('input[name="password"]').val();
		var loginObj = { "cmd": "LOGIN", "username": username, "password" : password};
		wsSend(JSON.stringify(loginObj));
	  	
	});
	$("#spinnerbg").show();



	$(".tab-header").first().find('div[class!="direct-link"]').each( function(i) {
		console.log(this);
		$(this).click(function() {
			$(".tab-header").first().find(".active").removeClass("active").addClass("inactive");
			$(this).addClass("active").removeClass("inactive");

			$(".tab-indicator").first().css("top", `calc(80px + ${i*50}px)`);


			$(".tab-content").first().find(".active").first().removeClass("active").addClass("inactive");
			$(".tab-content").first().find("div").eq(i).removeClass("inactive").addClass("active");


			console.log(this);
		});
	});

	$("#logout-btn").click(function() {
		$( "#dialog-logout-confirm" ).dialog( "open" );
		
	});

	$( "#dialog-logout-confirm" ).dialog({
	      resizable: false,
	      height: "auto",
	      width: 400,
	      modal: true,
	      autoOpen: false,
	      buttons: {
	        "Logout": function() {
	        	var cmd = { "cmd": "LOGOUT"};
    			wsSend(JSON.stringify(cmd));
	          	$( this ).dialog( "close" );
	        },
	        Cancel: function() {
	          	$( this ).dialog( "close" );
	        }
	      }
	    });
});


