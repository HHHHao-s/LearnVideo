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

var roomTableMap = new Map();
var connMap = new Map();
var uidMap = new Map();

class Client {
    constructor(uid, conn, roomId) {
        this.uid = uid;
        this.conn = conn;
        this.roomId = roomId;
    }
}

function handleJoin(conn, msg){
    
    var roomId = msg.roomId;
    var uid = msg.uid;

    client = new Client(uid, conn, roomId);
    connMap.set(conn, uid);
    uidMap.set(uid, client);
    
    console.log("handleJoin: "+uid+" join room "+roomId);
    roomMap = roomTableMap.get(roomId);
    if (roomMap == null){
        roomMap = new Map();
        roomTableMap.set(roomId, roomMap);
    }
    roomMap.set(uid, client);
    remoteIds = [];
    if(roomMap.size > 1){
        // 告知房间里的其他人有新人加入
        var newPeerMsg = {
            'cmd': SIGNAL_TYPE_NEW_PEER,
            'uid': uid
        };
        for (var [key, value] of roomMap) {
            if (key != uid){
                value.conn.sendText(JSON.stringify(newPeerMsg));
                remoteIds.push(key);
            }
        }
    }
    
    // 告知加入者对方有谁
    var respMsg = {
        'cmd': SIGNAL_TYPE_RESP_JOIN,
        'remoteIds': remoteIds
    };

    console.log("respMsg: "+JSON.stringify(respMsg));
    
    conn.sendText(JSON.stringify(respMsg));
}

function handleLeave(uid){
    console.log("handleLeave: "+uid);
    client = uidMap.get(uid);

    if (client != null){
        roomMap = roomTableMap.get(client.roomId);
        if (roomMap != null){
            roomMap.delete(uid);
            if (roomMap.size == 0){
                roomTableMap.delete(client.roomId);
            }else{
                // 告知房间里的其他人有人离开
                var peerLeaveMsg = {
                    'cmd': SIGNAL_TYPE_PEER_LEAVE,
                    'uid': uid
                };
                for (var [key, value] of roomMap) {
                    value.conn.sendText(JSON.stringify(peerLeaveMsg));
                }
            }
        }
    }
    connMap.delete(client.conn);
    uidMap.delete(uid);
    

    
    
}

function handleClose(conn){
    uid = connMap.get(conn);
    client = uidMap.get(uid);
    if (client != null){
        roomId= client.roomId; 
        roomMap = roomTableMap.get(roomId);
        if (roomMap != null){
            roomMap.delete(uid);
            if (roomMap.size == 0){
                roomTableMap.delete(roomId);
            }else{
                // 告知房间里的其他人有人离开
                var peerLeaveMsg = {
                    'cmd': SIGNAL_TYPE_PEER_LEAVE,
                    'uid': client.uid
                };
                for (var [key, value] of roomMap) {
                    value.conn.sendText(JSON.stringify(peerLeaveMsg));
                }
            }
        }
    }
    uidMap.delete(uid);
    connMap.delete(conn);
    console.log("handleClose: "+uid);
}

var server = ws.createServer(function(conn){
    console.log("New connection")
    conn.sendText("Hello world")
    conn.on("text", function (str) {
        console.log("Received "+str)
        var msg = JSON.parse(str);
        switch (msg.cmd){
            case SIGNAL_TYPE_JOIN:
                handleJoin(conn, msg);
                break;
            case SIGNAL_TYPE_LEAVE:
                handleLeave(msg.uid);
                break;
        }
    });
    conn.on("close", function (code, reason) {
        console.log("Connection closed:" + code+" "+reason)
        handleClose(conn);
        
    });

    conn.on("error", function (err) {
        console.log("handle err")
        console.log(err)
    });
}).listen(port);

