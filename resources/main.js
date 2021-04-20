console.log("Esp32MotherClock - OK");

var auth = false;



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



var ws = new WebSocket("wss://" + location.host + "/ws");

ws.onerror = function(event) {
	console.error("WebSocket error observed:", event);
	$("#spinnerbg").show();
};

ws.onmessage = function (event) {
	var msgObj = JSON.parse(event.data);
	console.log(msgObj);
	if(msgObj.cmd === "PING" ) {
		var answer = { "cmd": "PONG"};
		wsSend(JSON.stringify(answer));
	}

	if(msgObj.cmd === "STATUS" || msgObj.cmd === "LOGIN") {
		if(msgObj.auth && (! auth)) {
			$("#mainform").show();
			$("#loginform").hide();
			auth = true;
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





/*

function _class(name){
  return document.getElementsByClassName(name);
}

let tabPanes = _class("tab-header")[0].getElementsByTagName("div");

for(let i=0;i<tabPanes.length;i++){
  tabPanes[i].addEventListener("click",function(){
    _class("tab-header")[0].getElementsByClassName("active")[0].classList.remove("active");
    tabPanes[i].classList.add("active");
    
    _class("tab-indicator")[0].style.top = `calc(80px + ${i*50}px)`;
    
    _class("tab-content")[0].getElementsByClassName("active")[0].classList.remove("active");
    _class("tab-content")[0].getElementsByTagName("div")[i].classList.add("active");
    
  });
}


*/


/*	
$(window).bind('beforeunload', function(){
ws.send("close!");
ws.close();
});
*/