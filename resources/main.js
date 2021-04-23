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
var ws = null;
var pingTimeout = null;

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
		$(".removeable").remove();

		$("#menu-status").addClass("active");
		$("#tab-status").removeClass("inactive").addClass("active");
		$(".tab-indicator").first().css("top", `calc(80px + ${0*50}px)`);

		auth = false;
	} else if ((! msgObj.auth) && (! auth) && msgObj.cmd === "LOGIN") {
		$("#spinnerbg").hide();
		$("#loginform").shake();
	}
} 

function widgetChangeHandler(obj){

	if(getMapDataObj.mapObj.items[obj.currentTarget.name].hasOwnProperty('dependend')) {
		getMapDataObj.mapObj.items[obj.currentTarget.name].dependend.forEach(function (item, index) {
			if( item.type === "bool") {

				if(jQuery(event.target).is(":checked") ?  item.trueFalse : !item.trueFalse) { // inverse exclusive OR
					$("#widget-" + item.dependend).prop( "disabled", false );
				} else {
					$("#widget-" + item.dependend).prop( "disabled", true );
				}

			}
		});
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
		getMapDataObj.mapObj = JSON.parse(getMapDataObj.buffer);

		$.each(getMapDataObj.mapObj.sections, function(index, value ) { 
			$( "#menu-placeholder" ).before( "<div class=\"removeable\" id=\"menu-" + value.name + "\"><i class=\"ui-icon ui-icon-" + value.icon + "\"></i>" + value.caption + "</div>" );
           	$( "#tab-placeholder" ).before("<div class=\"inactive removeable menuselect\" id=\"tab-" + value.name + "\"><i class=\"ui-icon ui-icon-" + value.icon +"\"></i><h2>" + value.caption +"</h2><p></p></div>");
		});
		setupTabs();

		$.each(getMapDataObj.mapObj.items, function(key, value ) { 

			if(value.type === "text") {
				$("#tab-" + value.section).find("p").append("<div><label for=\"widget-" + key +"\">" + getMapDataObj.mapObj.i18n.en[key].caption +"</label><input name=\"" + key + "\" id=\"widget-" + key +"\" type=\"text\" value=\"\" /></div>");
			} else if(value.type === "password") {
				$("#tab-" + value.section).find("p").append("<div><label for=\"widget-" + key +"\">" + getMapDataObj.mapObj.i18n.en[key].caption +"</label><input name=\"" + key + "\" id=\"widget-" + key +"\" type=\"password\" value=\"\" /></div>");
			} else if(value.type === "textbox") {
				$("#tab-" + value.section).find("p").append("<div><label for=\"widget-" + key +"\">" + getMapDataObj.mapObj.i18n.en[key].caption +"</label><textarea name=\"" + key + "\" id=\"widget-" + key +"\" rows=\"20\" /></textarea></div>");
			} else if(value.type === "checkbox") {
				$("#tab-" + value.section).find("p").append("<div><label for=\"widget-" + key +"\">" + getMapDataObj.mapObj.i18n.en[key].caption +"</label><input name=\"" + key + "\" id=\"widget-" + key +"\" type=\"checkbox\" value=\"\" /></div>");
			} else {
				console.warn("omitting: " + key + ", type : " + value.type);
			}


		});

		$("input, textarea, select").change(function (obj){
			widgetChangeHandler(obj);
		});

		$.each(getMapDataObj.mapObj.items, function(key, value ) { 			
			if(value.hasOwnProperty('depends')){
				if(value.depends.match(/^boolDepened\(.+,(true|false)\)/)) {
					var depends = value.depends.match(/^boolDepened\((.+),(true|false)\)$/)[1];
					var trueFalse = ( value.depends.match(/^boolDepened\((.+),(true|false)\)$/)[2] === 'true');
					if(getMapDataObj.mapObj.items.hasOwnProperty(depends)){ 
						if(! getMapDataObj.mapObj.items[depends].hasOwnProperty('dependend')){ 
							getMapDataObj.mapObj.items[depends].dependend = []; 
						}
						getMapDataObj.mapObj.items[depends].dependend.push({"dependend": key, "trueFalse": trueFalse, "type" : "bool"});


						if($("#widget-" + depends).is(":checked") ?  trueFalse : !trueFalse) { // inverse exclusive OR
							$("#widget-" + key).prop( "disabled", false );
						} else {
							$("#widget-" + key).prop( "disabled", true );
						}


					}
				}
			}
		});


		$("#spinnerbg").hide();

	}

} 


function parseIncommingWS(event) {
	try {
		var msgObj = JSON.parse(event.data);
	//	console.log(msgObj);
		if(msgObj.cmd === "PING" ) {
			var answer = { "cmd": "PONG"};	
			wsSend(JSON.stringify(answer));
			if(pingTimeout != null) {
				clearTimeout(pingTimeout);
			}

			pingTimeout = setTimeout(function() {
				pingTimeout = null;
				console.error("No Ping, connection timeout");
				ws.close();
			},10000);
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

function connect() {
  ws = new WebSocket("wss://" + location.host + "/ws");

	ws.onopen = function() {
		var status = { "cmd": "GETSTATUS"};
		wsSend(JSON.stringify(status));
		$("#spinnerbg").hide();
	};

	ws.onmessage = function(e) {
		parseIncommingWS(event);
	};

	ws.onclose = function(e) {
		console.log('Socket is closed. Reconnect will be attempted in 5 seconds.', e.reason);
		setTimeout(function() {
		  connect();
		}, 5000);
	};

	ws.onerror = function(event) {
		console.error("WebSocket error observed:", event);
		$("#spinnerbg").show();
		ws.close();
	};

}

connect();




function wsSend(data) {
	ws.send(data);
}


function setupTabs() {
	$(".tab-header").first().find('div[class!="direct-link"]').filter('[class!="placeholder"]').each( function(i) {
		$(this).click(function() {
			$(".tab-header").first().find(".active").removeClass("active").addClass("inactive");
			$(this).addClass("active").removeClass("inactive");

			$(".tab-indicator").first().css("top", `calc(80px + ${i*50}px)`);


			$(".tab-content").first().find(".active").filter('[class*="menuselect"]').first().removeClass("active").addClass("inactive");
			$(".tab-content").first().find('div[class*="menuselect"]').eq(i).removeClass("inactive").addClass("active");

		});
	});
}

$( document ).ready(function() {
	$( "#loginform-form" ).submit(function( event ) {
		event.preventDefault();
		var username = $( "#loginform-form" ).find('input[name="username"]').val();
		var password = $( "#loginform-form" ).find('input[name="password"]').val();
		var loginObj = { "cmd": "LOGIN", "username": username, "password" : password};
		wsSend(JSON.stringify(loginObj));
		$("#spinnerbg").show();
	  	
	});
	$("#spinnerbg").show();


	setupTabs();


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


