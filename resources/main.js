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
		$('input[name="login_username"]').val('');
		$('input[name="login_password"]').val('');
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


function checkValidItems() {
	var ret = true;
	$.each(getMapDataObj.mapObj.items, function(key, item ) {
//		console.log(key, item.valid, item.active , item.relevant , (item.originalvalue !== item.value));

		if(item.valid && item.active && item.relevant && (item.originalvalue !== item.value)) {
			$("#save-btn").removeClass(" ui-state-disabled");
			return false;
		} else {
			$("#save-btn").addClass(" ui-state-disabled");
		}

	});

}

function widgetChangeHandler(obj){

	if(getMapDataObj.mapObj.items[obj.currentTarget.name].widget === 'checkbox') {

		getMapDataObj.mapObj.items[obj.currentTarget.name].value = jQuery(obj.currentTarget).is(":checked");
	} else {
		getMapDataObj.mapObj.items[obj.currentTarget.name].value = jQuery(obj.currentTarget).val();
	}

	if(getMapDataObj.mapObj.items[obj.currentTarget.name].hasOwnProperty('dependend')) {
		getMapDataObj.mapObj.items[obj.currentTarget.name].dependend.forEach(function (item, index) {
			if( item.type === "bool") {

				if(getMapDataObj.mapObj.items[obj.currentTarget.name].value ?  item.trueFalse : !item.trueFalse) { // inverse exclusive OR
					$("#widget-" + item.dependend).prop( "disabled", false );
					$("#widget-" + item.dependend).removeClass("disabledInput");
					getMapDataObj.mapObj.items[item.dependend].active = true;
				} else {
					$("#widget-" + item.dependend).prop( "disabled", true );
					$("#widget-" + item.dependend).addClass("disabledInput");
					getMapDataObj.mapObj.items[item.dependend].active = false;				}

			}
		});
	}

	if(getMapDataObj.mapObj.items[obj.currentTarget.name].hasOwnProperty('regexcheck')) {
		var regex = getMapDataObj.mapObj.items[obj.currentTarget.name].regexcheck;

		if(regex.match(/\$\{ref:.+\}/)) {
			var reference = regex.match(/\$\{ref:(.+)\}/)[1];
			if(getMapDataObj.mapObj.items.hasOwnProperty(reference)) {
				var referenceData = getMapDataObj.mapObj.items[reference].value;
				regex = regex.replace("${ref:" + reference + "}", referenceData != null ? referenceData : "");
			}
		}
		var regexedMatch = new RegExp(regex);
		if(getMapDataObj.mapObj.items[obj.currentTarget.name].value.match(regexedMatch)) {
			getMapDataObj.mapObj.items[obj.currentTarget.name].valid = true;

			jQuery(obj.currentTarget).removeClass("invalidInput");

		} else {
			getMapDataObj.mapObj.items[obj.currentTarget.name].valid = false;
			jQuery(obj.currentTarget).addClass("invalidInput");
		}
	}
	checkValidItems();
}

function setWidgetStateEnableDisable() {
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
						value.active = true;
						$("#widget-" + key).removeClass("disabledInput");
					} else {
						$("#widget-" + key).prop( "disabled", true );
						value.active = false;
						$("#widget-" + key).addClass("disabledInput");
					}


				}
			}
		}
	});
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

			if(value.widget === "text") {
				$("#tab-" + value.section).find("p").append("<div data-tip=\"" + getMapDataObj.mapObj.i18n.en[key].hint + "\"><label for=\"widget-" + key +"\">" + getMapDataObj.mapObj.i18n.en[key].caption +"</label><input name=\"" + key + "\" id=\"widget-" + key +"\" type=\"text\" value=\"\" /></div>");
			} else if(value.widget === "password") {
				$("#tab-" + value.section).find("p").append("<div><label for=\"widget-" + key +"\">" + getMapDataObj.mapObj.i18n.en[key].caption +"</label><input name=\"" + key + "\" id=\"widget-" + key +"\" type=\"password\" value=\"\" /></div>");
			} else if(value.widget === "textbox") {
				$("#tab-" + value.section).find("p").append("<div><label for=\"widget-" + key +"\">" + getMapDataObj.mapObj.i18n.en[key].caption +"</label><textarea name=\"" + key + "\" id=\"widget-" + key +"\" rows=\"20\" /></textarea></div>");
			} else if(value.widget === "checkbox") {
				$("#tab-" + value.section).find("p").append("<div><label for=\"widget-" + key +"\">" + getMapDataObj.mapObj.i18n.en[key].caption +"</label><input name=\"" + key + "\" id=\"widget-" + key +"\" type=\"checkbox\" value=\"\" /></div>");
			} else {
				console.warn("omitting: " + key + ", widget : " + value.widget);
			}


		});


		$("#mainform").find("input, textarea, select").on("input", function (obj) {
		  widgetChangeHandler(obj);
		});

		setWidgetStateEnableDisable();

		$.each(getMapDataObj.mapObj.items, function(key, value ) { 
			value.value = null;
			value.valid = true;
			value.active = true;
		});

		loadConf();

	}

} 


function updateWidgetsFromData() {
	$("#spinnerbg").show();
	$.each(getMapDataObj.mapObj.items, function(key, value ) {
		if(value.widget === "text") {
			$("#widget-" + key).val(value.value);
		} else if(value.widget === "password") {
			value.value = "12345678";
			value.originalvalue = "12345678";
			$("#widget-" + key).val(value.value);
		} else if(value.widget === "textbox") {
			$("#widget-" + key).val(value.value);
		} else if(value.widget === "checkbox") {
			$("#widget-" + key).prop("checked",value.value);
		}
	});
	setWidgetStateEnableDisable();

	$("#spinnerbg").hide();
}

function getcfg(msgObj) {
	$("#spinnerbg").show();

	msgObj.items.forEach(function (currentItem) {
		if(getMapDataObj.mapObj.items.hasOwnProperty(currentItem.name)) {
			getMapDataObj.mapObj.items[currentItem.name].originalvalue = currentItem.value;
			getMapDataObj.mapObj.items[currentItem.name].value = currentItem.value;
		}
		
	});


	updateWidgetsFromData();

	$("#spinnerbg").hide();

}

function loadConf() {
	$("#spinnerbg").show();
	var confToGet = new Array();
	$.each(getMapDataObj.mapObj.items, function(key, value ) {
    	if(value.omitsend) { return; }
    	confToGet.push(key);
	});

	var confGetCMD = { "cmd": "GETCFG", "items": confToGet};
	wsSend(JSON.stringify(confGetCMD));

	$("#spinnerbg").hide();

}


function save() {
	var itemsToSend = new Array();

	$.each(getMapDataObj.mapObj.items, function(key, value ) {
		if(value.value !== value.originalvalue && value.relevant) {
			itemsToSend.push({"name": key, "value": value.value});
		}

	});

	if(itemsToSend.length > 0) {
		var setCfgCmd = { "cmd": "SETCFG", "items": itemsToSend};
		wsSend(JSON.stringify(setCfgCmd));
	}


}


function parseIncommingWS(event) {
	try {
		var msgObj = JSON.parse(event.data);
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
		if(((msgObj.cmd === "STATUS" && msgObj.hasOwnProperty("auth")))|| msgObj.cmd === "LOGIN") {
			login(msgObj);
		}

		if((msgObj.cmd === "STATUS" && msgObj.hasOwnProperty("configdirty"))) {
			if(msgObj.configdirty) {
				$("#save-btn").addClass(" ui-state-disabled");
				loadConf();
			}
		}



		if(msgObj.cmd === "GETMAP" ) {
			getmap(msgObj);
		}

		if(msgObj.cmd === "GETCFG" ) {
			getcfg(msgObj);
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
//	$(".tab-header").first().find('div[class!="direct-link"]').filter('[class!="placeholder"]').each( function(i) {
	$(".tab-header").first().find('div:not(.direct-link)').filter('[class!="placeholder"]').each( function(i) {
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
		var username = $( "#loginform-form" ).find('input[name="login_username"]').val();
		var password = $( "#loginform-form" ).find('input[name="login_password"]').val();
		var loginObj = { "cmd": "LOGIN", "username": username, "password" : password};
		wsSend(JSON.stringify(loginObj));
		$("#spinnerbg").show();
	  	
	});
	$("#spinnerbg").show();


	setupTabs();



	$("#logout-btn").click(function() {
		$( "#dialog-logout-confirm" ).dialog( "open" );

	});

	$("#save-btn").click(function() {
		save();
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
	       		$("#save-btn").addClass(" ui-state-disabled");
	        },
	        Cancel: function() {
	          	$( this ).dialog( "close" );
	        }
	      }
	    });
});


