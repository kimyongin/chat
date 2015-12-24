new function() {
	var ws = null;
	var connected = false;

	var serverUrl;
	var connectionStatus;
	var sendMessage;
	
	var connectButton;
	var disconnectButton; 
	var sendButton;

	var open = function() {
		var url = serverUrl.val();
		ws = new WebSocket(url);
		ws.onopen = onOpen;
		ws.onclose = onClose;
		ws.onmessage = onMessage;
		ws.onerror = onError;

		connectionStatus.text('OPENING ...');
		serverUrl.attr('disabled', 'disabled');
		connectButton.hide();
		disconnectButton.show();
	}
	
	var close = function() {
		if (ws) {
			console.log('CLOSING ...');
			ws.close();
		}
		connected = false;
		connectionStatus.text('CLOSED');

		serverUrl.removeAttr('disabled');
		connectButton.show();
		disconnectButton.hide();
		sendMessage.attr('disabled', 'disabled');
		sendButton.attr('disabled', 'disabled');
	}
	
	var clearLog = function() {
		$('#messages').html('');
	}
	
	var onOpen = function() {
		console.log('OPENED: ' + serverUrl.val());
		connected = true;
		connectionStatus.text('OPENED');
		sendMessage.removeAttr('disabled');
		sendButton.removeAttr('disabled');
	};
	
	var onClose = function() {
		console.log('CLOSED: ' + serverUrl.val());
		ws = null;
	};
	
	var onMessage = function(event) {
		var data = event.data;
		addMessage(data);
	};
	
	var onError = function(event) {
		alert(event.data);
	}
	
	var addMessage = function(data, type) {
		var msg = $('<pre>').text(data);
		if (type === 'SENT') {
			msg.addClass('sent');
		}
		var messages = $('#messages');
		messages.append(msg);
		
		var msgBox = messages.get(0);
		while (msgBox.childNodes.length > 1000) {
			msgBox.removeChild(msgBox.firstChild);
		}
		msgBox.scrollTop = msgBox.scrollHeight;
	}

	WebSocketClient = {
		init: function() {
			serverUrl = $('#serverUrl');
			connectionStatus = $('#connectionStatus');
			sendMessage = $('#sendMessage');
			
			connectButton = $('#connectButton');
			disconnectButton = $('#disconnectButton'); 
			regButton =  $('#regButton');
			sendButton = $('#sendButton');
			thxsButton = $('#thxsButton');
			calcButton = $('#calcButton');
			testButton = $('#testButton');
			
			connectButton.click(function(e) {
				close();
				open();
			});
		
			disconnectButton.click(function(e) {
				close();
			});
			
			sendButton.click(function(e) {
				var msg = $('#sendMessage').val();
				addMessage(msg, 'SENT');
				ws.send(msg);
			});
			
			regButton.click(function(e) {
				$('#sendMessage').val('{"event":"regist","data":{"user_name":"hongkildong"}}');
				var msg = $('#sendMessage').val();
				addMessage(msg, 'SENT');
				ws.send(msg);
			});
			
			thxsButton.click(function(e) {
				$('#sendMessage').val("thanks");
				var msg = $('#sendMessage').val();
				addMessage(msg, 'SENT');
				ws.send(msg);
			});
			
			calcButton.click(function(e) {
				$('#sendMessage').val('{"event":"calc","data":{"val1":"1", "val2":"2", "op":"+"}}');
				var msg = $('#sendMessage').val();
				addMessage(msg, 'SENT');
				ws.send(msg);
			});
			
			testButton.click(function(e) {
				for(var i = 0; i < 10; i++)
				{
					worker = new Worker("worker.js");  //새로운 워커(객체)를 생성한다 
					worker.onmessage = function(event){ //워커로부터 전달되는 메시지를 받는다
						addMessage(event.data);
					};
					var workerID = Math.floor(Math.random() * 1000) + 1;
					var obj = {"url":serverUrl.val(), "name":"Worker " + workerID};
					worker.postMessage(obj); //워커에게 메시지를 전달한다    
				}
			});
			
			$('#clearMessage').click(function(e) {
				clearLog();
			});
			
			var isCtrl;
			sendMessage.keyup(function (e) {
				if(e.which == 17) isCtrl=false;
			}).keydown(function (e) {
				if(e.which == 17) isCtrl=true;
				if(e.which == 13 && isCtrl == true) {
					sendButton.click();
					return false;
				}
			});
		}
	};
}

$(function() {
	WebSocketClient.init();
});