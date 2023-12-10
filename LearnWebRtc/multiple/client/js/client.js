'use strict';

var localVideo = document.getElementById('localVideo');
var remoteVideo = document.getElementById('remoteVideo');

var btnConn = document.querySelector('button#JoinRoom');
var btnLeave = document.querySelector('button#LeaveRoom');

var localStream;


const SIGNAL_TYPE_JOIN = "join";
const SIGNAL_TYPE_RESP_JOIN = "resp-join"; // 告知加入者对方是谁
const SIGNAL_TYPE_NEW_PEER = "new-peer"; // 告知房间里的其他人有新人加入
const SIGNAL_TYPE_PEER_LEAVE = "peer-leave"; // 告知房间里的其他人有人离开
const SIGNAL_TYPE_OFFER = "offer";
const SIGNAL_TYPE_ANSWER = "answer";
const SIGNAL_TYPE_CANDIDATE = "candidate";
const SIGNAL_TYPE_LEAVE = "leave";

var localUserId = Math.random().toString(36).slice(-8);
var remoteUserId = null;
var roomId = 0;

function doJoin(roomId){
        var jsonMsg = {
                'cmd': SIGNAL_TYPE_JOIN,
                'roomId': roomId,
                'uid'   : localUserId
        };
        zeroRTCEngine.sendMessage(JSON.stringify(jsonMsg));
        console.log('doJoin: ' + JSON.stringify(jsonMsg));
}

function openLocalStream(stream) {
        console.log('openLocalStream');
        

        localVideo.srcObject = stream;
        localStream = stream;
}

var zeroRTCEngine;

var ZeroRTCEngine = function(wsUrl) {
        this.init(wsUrl);    
        zeroRTCEngine = this;
        return this;
}

ZeroRTCEngine.prototype.init= function(wsUrl) {
        this.wsUrl = wsUrl;
        // websocket
        this.signaling = null;
}

ZeroRTCEngine.prototype.createSignaling = function() {
        zeroRTCEngine = this;

        zeroRTCEngine.signaling = new WebSocket(this.wsUrl);

        zeroRTCEngine.signaling.onopen = function() {
                zeroRTCEngine.onOpen();
        };

        zeroRTCEngine.signaling.onmessage = function(event) {
                
                zeroRTCEngine.onMessage(event);
        };

        zeroRTCEngine.signaling.onclose = function(event) {
                zeroRTCEngine.onClose(event);
        };

        zeroRTCEngine.signaling.onerror = function(event) {
                zeroRTCEngine.onError(event);
        };

}

ZeroRTCEngine.prototype.sendMessage = function(msg) {
        this.signaling.send(msg);
}

ZeroRTCEngine.prototype.handleRespJoin = function(msg){
        console.log('handleRespJoin: ' + JSON.stringify(msg));
        var msgremoteId = msg.remoteId;
        if(msgremoteId != null){
                remoteUserId = msgremoteId;
        }
}

function doOffer(remoteId){
        console.log('doOffer: ' + remoteId);
}





ZeroRTCEngine.prototype.handleNewPeer = function(msg){
        console.log('handleNewPeer: ' + JSON.stringify(msg));
        var remoteId = msg.uid;
        remoteUserId = remoteId;
        doOffer(remoteId);
}

ZeroRTCEngine.prototype.handlePeerLeave = function(msg){
        console.log('handlePeerLeave: ' + JSON.stringify(msg));
        var remoteId = msg.uid;
        remoteUserId = null;
        remoteVideo.srcObject = null;

}

ZeroRTCEngine.prototype.onMessage = function(event) {
        console.log('onMessage'+event.data);
        var msg = JSON.parse(event.data);
        switch (msg.cmd){
                case SIGNAL_TYPE_RESP_JOIN:
                        this.handleRespJoin(msg);
                        break;
                case SIGNAL_TYPE_NEW_PEER:
                        this.handleNewPeer(msg);
                        break;
                case SIGNAL_TYPE_PEER_LEAVE:
                        this.handlePeerLeave(msg);
                        break;
                case SIGNAL_TYPE_OFFER:
                        this.handleOffer(msg);
                        break;
                case SIGNAL_TYPE_ANSWER:
                        this.handleAnswer(msg);
                        break;
                case SIGNAL_TYPE_CANDIDATE:
                        this.handleCandidate(msg);
                        break;
                case SIGNAL_TYPE_LEAVE:
                        this.handleLeave(msg);
                        break;
                default:
                        break;
        }
       
}

ZeroRTCEngine.prototype.onClose = function(event) {
        console.log('onClose' + event.data);
}

ZeroRTCEngine.prototype.onOpen = function() {
        console.log('onOpen');
        
}

ZeroRTCEngine.prototype.onError = function(event) {
        console.log('onError' + event.data);
}


function initZeroRTCEngine() {
        zeroRTCEngine = new ZeroRTCEngine();
        zeroRTCEngine.on('onAddRemoteStream', function(stream) {
                remoteVideo.srcObject = stream;
        });
        zeroRTCEngine.on('onRemoveRemoteStream', function(stream) {
                remoteVideo.srcObject = null;
        });
}

function initLocalStream() {
        navigator.mediaDevices.getUserMedia({
            audio: true,
                video: true  
        })
        .then(openLocalStream)
        .catch(function(e) {
                alert('getUserMedia() error: ' + e.name);
        });
}

zeroRTCEngine = new ZeroRTCEngine('ws://localhost:8099');
zeroRTCEngine.createSignaling();

btnConn.onclick = function () {
        console.log('Join Room');
        roomId = document.getElementById('roomId').value;
        if(roomId == null || roomId == ''){
                alert('请输入房间号');
                return;
        }else{
                doJoin(roomId);
        }
        // 初始化本地码流
        // initLocalStream();
}

function doLeave(){
        var jsonMsg = {
                'cmd': SIGNAL_TYPE_LEAVE,
                'roomId': roomId,
                'uid'   : localUserId
        };
        zeroRTCEngine.sendMessage(JSON.stringify(jsonMsg));
        console.log('doLeave: ' + JSON.stringify(jsonMsg));

}

btnLeave = document.getElementById('LeaveRoom');



btnLeave.onclick = function () {
        console.log('leave Room');
        
        doLeave();
}