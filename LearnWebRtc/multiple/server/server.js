var ws = require("nodejs-websocket");
var port = 8099;

const SIGNAL_TYPE_JOIN = "join";
const SIGNAL_TYPE_RESP_JOIN = "resp-join"; // 告知加入者对方是谁
const SIGNAL_TYPE_NEW_PEER = "new-peer"; // 告知房间里的其他人有新人加入
const SIGNAL_TYPE_PEER_LEAVE = "peer-leave"; // 告知房间里的其他人有人离开
const SIGNAL_TYPE_OFFER = "offer";
const SIGNAL_TYPE_ANSWER = "answer";
const SIGNAL_TYPE_CANDIDATE = "candidate";
const SIGNAL_TYPE_LEAVE = "leave";

roomTableMap = new Map();

class Client{
    constructor(uid, conn, roomId){
        
        this.uid = uid;
        this.conn = conn
        this.roomId = roomId;
    }
}

function handleJoin(jsonMsg, conn){

    roomId = jsonMsg.roomId;
    uid = jsonMsg.uid;


    var roomTable = roomTableMap.get(roomId);
    if(roomTable == null){
        roomTable = new Map();
        
    }
    if(roomTable.size >=2){
        // room is full
        // add new signal to support multi-peer
        return ;
    }
    var client = new Client(uid, conn, roomId);
    roomTable.set(uid, client);
    roomTableMap.set(roomId, roomTable);
        
    
    
    if(roomTable.size > 1){
        console.log("room is 2");
        var clients = roomTable.values();
        for(var client of clients){
            // console.log("client "+client.uid);

            if(client.uid == uid){
                var respMsgtoCur = {
                    "cmd": SIGNAL_TYPE_RESP_JOIN,
                    "remoteUid": client.uid
                }
                conn.sendText(JSON.stringify(respMsgtoCur));
                continue;
            }
            var respMsg = {
                "cmd": SIGNAL_TYPE_NEW_PEER,
                "uid": uid
            }
            
            client.conn.sendText(JSON.stringify(respMsg));
            
        }
    }else{
        console.log("room is empty");
        var respMsg = {
            "cmd": SIGNAL_TYPE_RESP_JOIN,
            "remoteUid": null
        }
        conn.sendText(JSON.stringify(respMsg));
    }


}

function handleLeave(jsonMsg, conn){
    uid = jsonMsg.uid;
    roomId = jsonMsg.roomId;
    var roomTable = roomTableMap.get(roomId);
    if(roomTable == null){
        console.log("roomTable is null");
        return;
    }
    var client = roomTable.get(uid);
    if(client == null){
        console.log("client is null");
        return;
    }
    roomTable.delete(uid);
    roomTableMap.set(roomId, roomTable);
    if(roomTable.size > 0){
        var clients = roomTable.values();
        var respMsg = {
            "cmd": SIGNAL_TYPE_PEER_LEAVE,
            "uid": uid
        }
        respMsgjson = JSON.stringify(respMsg);
        for(var client of clients){
            
            client.conn.sendText(respMsgjson);
        }
    }else if(roomTable.size == 0){
        roomTableMap.delete(roomId);
    }
}

function handleOffer(jsonMsg, conn){

    console.log("handleOffer");
    uid = jsonMsg.uid;
    roomId = jsonMsg.roomId;
    var roomTable = roomTableMap.get(roomId);
    if(roomTable == null){
        console.log("roomTable is null");
        return;
    }
    var client = roomTable.get(uid);
    if(client == null){
        console.log("client is null");
        return;
    }
    var remoteId = jsonMsg.remoteId;
    var remoteClient = roomTable.get(remoteId);
    if(remoteClient == null){
        console.log("remoteClient is null");
        return;
    }
    var respMsg = {
        "cmd": SIGNAL_TYPE_OFFER,
        "uid": uid,
        "offer": jsonMsg.offer
    };
    remoteClient.conn.sendText(JSON.stringify(respMsg));
}

function handleAnswer(jsonMsg, conn){
    console.log("handleAnswer");
    uid = jsonMsg.uid;
    roomId = jsonMsg.roomId;
    var roomTable = roomTableMap.get(roomId);
    if(roomTable == null){
        console.log("roomTable is null");
        return;
    }
    var client = roomTable.get(uid);
    if(client == null){
        console.log("client is null");
        return;
    }
    var remoteId = jsonMsg.remoteId;
    var remoteClient = roomTable.get(remoteId);
    if(remoteClient == null){
        console.log("remoteClient is null");
        return;
    }
    var respMsg = {
        "cmd": SIGNAL_TYPE_ANSWER,
        "uid": uid,
        "answer": jsonMsg.answer
    };
    remoteClient.conn.sendText(JSON.stringify(respMsg));
}

function handleCandidate(jsonMsg, conn){
    console.log("handleCandidate");
    uid = jsonMsg.uid;
    roomId = jsonMsg.roomId;
    var roomTable = roomTableMap.get(roomId);
    if(roomTable == null){
        console.log("roomTable is null");
        return;
    }
    var client = roomTable.get(uid);
    if(client == null){
        console.log("client is null");
        return;
    }
    var remoteId = jsonMsg.remoteId;
    var remoteClient = roomTable.get(remoteId);
    if(remoteClient == null){
        console.log("remoteClient is null");
        return;
    }
    var respMsg = {
        "cmd": SIGNAL_TYPE_CANDIDATE,
        "uid": uid,
        "candidate": jsonMsg.candidate
    };
    remoteClient.conn.sendText(JSON.stringify(respMsg));
}

var server = ws.createServer(function(conn){

    console.log("New connection");
   
    conn.on("text", function (str) {
        
        console.log("Received "+str);

        var jsonMsg = JSON.parse(str);
        var cmd = jsonMsg.cmd;
        switch(cmd){
            case SIGNAL_TYPE_JOIN:{
                handleJoin(jsonMsg, conn);
                
                break;
            }
            case SIGNAL_TYPE_LEAVE:{
                handleLeave(jsonMsg, conn);
                break;
            }
            case SIGNAL_TYPE_OFFER:{
                handleOffer(jsonMsg, conn);
                break;
            }
            case SIGNAL_TYPE_ANSWER:{
                handleAnswer(jsonMsg, conn);
                break;
            }
            case SIGNAL_TYPE_CANDIDATE:{
                handleCandidate(jsonMsg, conn);
                break;
            }
            default:
                break;

        }

    });
    

    conn.on("close", function (code, reason) {
        console.log("Connection closed");
    });

    conn.on("error", function (err) {
        console.log("handle err");
        console.log(err);
    }); 

}).listen(port);
