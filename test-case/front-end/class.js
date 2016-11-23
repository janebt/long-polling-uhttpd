function listenException()
{
	var queryMDID;
	var STOP_INTERVAL = 2000;

/*
	请求报文:
	{
		motion_detection": {"name": "motion_status"},
		"method": "get"
	}
	响应报文:
	{
        "motion_detection": {"motion_status": { "motion_exception_status": "0" } }，
		"error_code": 0
	}
 */
	this.questMDHandle = function()
	{
		var FILE_NAME = uciMotionDetection.fileName;
		var MOTION_STATUS = uciMotionDetection.secName.motionStatus;
		var STATUS = uciMotionDetection.optName.Status;


		var reqData = {};
		reqData[FILE_NAME] = {};
		reqData[FILE_NAME][NAME] = MOTION_STATUS;

		clearInterval( queryMDID );
		queryMDID= setInterval(function(){
			$.query(reqData, function(result){

					if ("1" == parseInt(result[FILE_NAME][MOTION_STATUS][STATUS]))
					{
						showToast("other", {noteStr:label.navigateToIp});
						$.setTimeout(function(){
							closeToast();
						}, STOP_INTERVAL);
					}
					else if ("2" == parseInt(result[FILE_NAME][MOTION_STATUS][STATUS]))
					{
						showToast("other", {noteStr:label.dhcpFailed});
						$.setTimeout(function(){
							closeToast();
						}, STOP_INTERVAL);
					}
					else
					{

					}
			});
		}, 5000);
	}

	this.listenMDHandle = function()
	{
		// 创建一个Socket实例
		var socket = new WebSocket('ws://192.168.1.60:8890');
		// 打开Socket
		socket.onopen = function(event)
		{
			var str= "{\"motion_detection\":{\"name\": \"motion_status\"},\"method\": \"get\"}"
			// 发送一个初始化消息
			socket.send(str);
		}

		// 监听消息
		socket.onmessage = function(event)
		{
			console.log('Client received a message',event);

			clearInterval( queryMDID );

			if ("status:1" == event.data)
			{
				showToast("other", {noteStr:label.navigateToIp});
				$.setTimeout(function(){
					closeToast();
				}, STOP_INTERVAL);
			}
			else if ("status:2" == event.data)
			{
				showToast("other", {noteStr:label.dhcpFailed});
				$.setTimeout(function(){
					closeToast();
				}, STOP_INTERVAL);
			}
			else
			{
			}

		};

		// 监听Socket的关闭
		socket.onclose = function(event)
		{
			console.log('Client notified socket has closed',event);
		};

		// 关闭Socket....
		//socket.close()
	}

	this.listenExceptionHandle = function()
	{
		var supportWebsocket;

		if(!("WebSocket" in window))
		{
			supportWebsocket = false;
			console.log("WebSocket not in window");
			questMDHandle();
		}
		else
		{
			supportWebsocket = true;
			console.log("WebSocket in window");
			listenMDHandle();
		}
	}
}

function updater ()
{
	this.longPolling = function()
	{
		$.ajax({url: "/cgi-bin/get_motion_detection_status",
		                type: "POST",
		                dataType: "text",
		                data: "quest exception status",
		                timeout: 10*60*1000,
		                success:function(data){
	                    },
	                    complete:function(XMLHttpRequest,textStatus){
	                            if(XMLHttpRequest.readyState=="4")
	                            {
	                                console.log(XMLHttpRequest.responseText);
	                            }
	                    },
		                //success: updater.onSuccess,
		                error: updater.onError});
	};

	this.onSuccess = function(data)
	{
/*
        try{
            alert("data :"+data);
        }
        catch(e)
        {
            updater.onError();
            return;
        }
        interval = window.setTimeout(updater.longPolling, 0);
*/
    };

    this.onError = function()
    {
        alert("Poll error;");
    };
}

(function(){
	listenException.call(window);
	updater.call(window);
})();

