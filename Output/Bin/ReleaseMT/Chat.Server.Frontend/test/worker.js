var ws = null;
var obj = null;
var sendcount = 0;
var readcount = 0;

var wsopen = function() {
	ws = new WebSocket(obj.url);
	ws.onopen = onOpen;
	ws.onclose = onClose;
	ws.onmessage = onReceive;
	ws.onerror = onError;
}

var wsclose = function() {
	if (ws) {
		ws.close(); // socket close
	}
}

var onOpen = function() {
	sendcount = Math.floor(Math.random() * 50) + 1;
	for(var index = 0; index < sendcount; index++)
	{
		ws.send("thanks");
	}
	sendcount = sendcount - parseInt(sendcount * 0.1); // 메세지가 도착하기전에 종료시키기 위해서
};

var onClose = function() {
	ws = null;
	close(); // worker close
};

var onReceive = function(event) {
	readcount++;
	postMessage(obj.name + " : " + event.data);
	if(readcount == sendcount){
		wsclose();
		close(); // worker close
	}
};

var onError = function(event) {
	alert(event.data);
	close(); // worker close
}

onmessage = function(event){
	obj = event.data;	
	var totalCount = Math.floor(Math.random() * 25) + 1;
	wsopen();
}