'use strict';

var localVideo = document.getElementById('localVideo');
var remoteVideo = document.getElementById('remoteVideo');

var btnConn = document.querySelector('button#JoinRoom');
var btnLeave = document.querySelector('button#LeaveRoom');

var localStream;
var pc;

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

async function doJoin(roomId){
        // 异步等待本地流初始化完成
        await initLocalStream();

        var jsonMsg = {
                'cmd': SIGNAL_TYPE_JOIN,
                'roomId': roomId,
                'uid'   : localUserId
        };
        console.log('doJoin: ' + JSON.stringify(jsonMsg));
        zeroRTCEngine.sendMessage(JSON.stringify(jsonMsg));
        
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

function createPeerConnection() {
        console.log('createPeerConnection');
        pc = new RTCPeerConnection();

        pc.onicecandidate = function(event) {
                if (event.candidate) {
                        console.log(event.candidate);
                        console.log(JSON.stringify(event.candidate));
                        var jsonMsg = {
                                'cmd': SIGNAL_TYPE_CANDIDATE,
                                'roomId': roomId,
                                'uid'   : localUserId,
                                'candidate': event.candidate,
                                'remoteId': remoteUserId
                        };
                        zeroRTCEngine.sendMessage(JSON.stringify(jsonMsg));
                        console.log('createPeerConnection: ' + JSON.stringify(jsonMsg));
                }else{
                        console.log('End of candidates.');
                }
        
        }

        pc.ontrack = function(event) {
                console.log('ontrack');
                remoteVideo.srcObject = event.streams[0];

        }

        pc.onremovestream = function(event) {
                console.log('onremovestream');
                zeroRTCEngine.emit('onRemoveRemoteStream', event.streams[0]);
        }

        localStream.getTracks().forEach(function(track) {
                pc.addTrack(track, localStream);
        });

}

function setDescAndSendOffer(offer){

        pc.setLocalDescription(offer);
        console.log(offer);

        var jsonMsg = {
                'cmd': SIGNAL_TYPE_OFFER,
                'roomId': roomId,
                'uid'   : localUserId,
                'offer': offer,
                'remoteId': remoteUserId

        };
        zeroRTCEngine.sendMessage(JSON.stringify(jsonMsg));
        console.log('setDescAndSendOffer: ' + JSON.stringify(jsonMsg));

}

function doOffer(remoteId){
        console.log('doOffer: ' + remoteId);

        createPeerConnection();

        pc.createOffer().then(setDescAndSendOffer).catch(function(err) {
                console.log(err);
        
        });

}

function setDescAndSendAnswer(Answer){

        pc.setLocalDescription(Answer);
        console.log(Answer);
        console.log(JSON.stringify(Answer));
        var jsonMsg = {
                'cmd': SIGNAL_TYPE_ANSWER,
                'roomId': roomId,
                'uid'   : localUserId,
                'answer': Answer,
                'remoteId': remoteUserId

        };
        zeroRTCEngine.sendMessage(JSON.stringify(jsonMsg));
        console.log('setDescAndSendOffer: ' + JSON.stringify(jsonMsg));

}

function doAnswer(remoteId){
        console.log('doOffer: ' + remoteId);


        pc.createAnswer().then(setDescAndSendAnswer).catch(function(err) {
                console.log(err);
        
        });

}


ZeroRTCEngine.prototype.handleOffer = function(msg){

        var offer = msg.offer;
        remoteUserId = msg.uid;
        console.log('handleOffer: ' + JSON.stringify(msg));
        createPeerConnection();
        pc.setRemoteDescription(new RTCSessionDescription(offer));

        doAnswer(remoteUserId);

}

ZeroRTCEngine.prototype.handleAnswer= function(msg){
        console.log('handleAnswer: ' + JSON.stringify(msg));
        var answer = msg.answer;
        pc.setRemoteDescription(new RTCSessionDescription(answer));

}

 ZeroRTCEngine.prototype.handleCandidate = function(msg){
        console.log('handleCandidate: ' + JSON.stringify(msg));
        var candidate = msg.candidate;
        pc.addIceCandidate(new RTCIceCandidate(candidate)).catch(function(err) {
                console.log(err);
        
        });


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

async function initLocalStream() {
        let response = await  navigator.mediaDevices.getUserMedia({
            audio: true,
                video: true  
        });
        localStream = response;
        localVideo.srcObject = localStream;
        console.log('initLocalStream');
        
}

zeroRTCEngine = new ZeroRTCEngine('ws://localhost:8099');
zeroRTCEngine.createSignaling();

btnConn.onclick = function () {
        console.log('Join Room');
        roomId = document.getElementById('roomId').value;
        // 初始化本地码流
        
        if(roomId == null || roomId == ''){
                alert('请输入房间号');
                return;
        }else{
                
                
                doJoin(roomId);
                
        }
        
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