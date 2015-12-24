'use strict';

function isScrollEnd() {
    var offsetValue = $('.chat-inner').offset().top;
    var chatOffset = Math.abs(offsetValue);
    var myOffset = $('.chat-inner').outerHeight() - $('.header').outerHeight() - $('.chat-container').outerHeight();

    if (myOffset < 0)
        return true;
    if (chatOffset + 100 < myOffset) {
        return false;
    } else {
        return true;
    }
}

function scrollAdjust() {
    $('.chat-container').scrollTop($('.chat-inner').outerHeight());
}

function focusAdjust() {
    $('#chat-content-text').focus();
}

function chatLayoutInit() {
    var $obj = $('.chat-container');
    var _height = $(window).height() - $('.header').outerHeight() - $('.chat-content-input-wrap').outerHeight();

    $obj.css({
        overflowY: 'auto',
        height: _height + 'px'
    });
}

function inviteLayoutInit() {
    var $obj = $('.invite-list ul');
    var _height = $(window).height() - $('.invite-popup h2').outerHeight() - $('.invite-popup .group-user-detail').outerHeight();

    $obj.css({
        overflowY: 'auto',
        height: _height + 'px'
    });
}

function triggerClick(_this, _name) {
    var $this = $(_this);
    var $obj = $('[name=' + _name + ']');

    if ($obj.is(':visible')) {
        $this.find($obj).trigger('click');
    }
}

function printNewMessage(msg) {
    $('.chat-message-new').remove();
    $('.chat-message-alert').remove();
    var printHtml = '<div class="chat-message-new" onclick="scrollAdjust();"><span>' + msg + '</span></div>';
    $('.chat-container').append(printHtml);
    $('.chat-message-new').delay(2000).fadeOut(500);
}

function printAlertMessage(msg) {
    $('.chat-message-new').remove();
    $('.chat-message-alert').remove();
    var printHtml = '<div class="chat-message-alert"><span>' + msg + '</span></div>';
    $('.chat-container').append(printHtml);
    $('.chat-message-alert').delay(2000).fadeOut(500);
}

$(window).resize(function () {
    chatLayoutInit();
    inviteLayoutInit();
});

function listOpen(id) {
    var $obj = $(id);

    $obj.css({
        visibility: 'hidden'
    });

    var _width = $obj.width();

    $obj.css({
        left: '-' + _width + 'px',
        visibility: 'visible',
        display: 'block'
    });

    $obj.animate({
        left: 0
    }, 200, function () {
        inviteLayoutInit();
    });
}

function userListOpen() {
    listOpen('#userPopup');
    modalShow(userListClose);
}

function userListClose() {
    var $obj = $('#userPopup');
    $obj.hide();
    modalClose();
};

function inviteListOpen() {
    listOpen('#invitePopup');
    modalShow(function () {
        var $obj = $('#invitePopup');
        $obj.hide();
        modalClose();
    });
}

function inviteListClose() {
    var $obj = $('#invitePopup');
    $obj.hide();
    modalClose();
};

var myChat = angular.module('myChatRoom', ['ui.utils', 'ngRoute', 'ngWebsocket', 'ui.bootstrap', 'ui.bootstrap.contextMenu']);

myChat.config(function($locationProvider) {
    $locationProvider.html5Mode({
        enabled: true,
        requireBase: false
    });
});

angular.module('myChatRoom').filter('time', function ($filter) {
    return function (input) {
        if (input == null) { return ""; }

        var _date = $filter('date')(new Date(input), 'HH:mm:ss');

        return _date.toUpperCase();

    };
});

myChat.controller('myChatRoomController', function ($scope, $http, $location, $route, $websocket, $rootScope, $modal, $log) {

    $scope.users = [];
    $scope.friends = [];
    $scope.messages = [];
    $scope.lastSendSeq = 0;
    $scope.startReadSeq = 0;
    $scope.roomNickName = ''

    $scope.menuOptions = [];
    $scope.menuOptions.push(['대화상대 보기', function ($itemScope) { userListOpen(); }]);
    $scope.menuOptions.push(['친구 초대하기', function ($itemScope) { $scope.invitePopup(); }]);
    $scope.menuOptions.push(['채팅방 나가기', function ($itemScope) { $scope.leave(); }]);
    $scope.menuOptions.push(['창 닫기', function ($itemScope) { $scope.close(); }]);

    $scope.$watch('friends', function (newValue, oldValue) {
        $scope.countCheckedFriend = countIf($scope.friends, 'is_checked', true);
    }, true);

    var wsUrl = "ws://" + $location.host() + ":" + $location.port() + "/chat/ws";
    var search = $location.search();
    $scope.RoomName = search.room_name;
    $scope.MyName = search.user_name;

    var ws = $websocket.$new({
        url: wsUrl,
        reconnect: false
    });

    chatLayoutInit();
    focusAdjust();

    /* open, close ----------------------------------------------- */
    ws.$on('$open', function () {
        $scope.$apply();
        var data = {
            room_name: encodeURIComponent($scope.RoomName),
            user_name: encodeURIComponent($scope.MyName)
        };
        ws.$emit('join', data);
    });

    ws.$on('$close', function () {
        $scope.setState('close', "서버와의 연결이 해제 되었습니다.", "채팅방에 다시 접속 해주세요.");
        $scope.$apply();
    });

    /* room ----------------------------------------------- */
    ws.$on('roomname', function (data) {
        if (data.user_name == encodeURIComponent($scope.MyName)) {
            $scope.roomNickName = decodeURIComponent(data.room_nick_name);
            var room_title = $scope.roomNickName;            
            if ($scope.users.length == 0) {
                room_title = room_title + ' (비어있음)';
            } else {
                var totalCnt = $scope.users.length + 1;
                if (totalCnt > 2) {
                    room_title = room_title + ' (' + totalCnt + ')';
                }
            }
            $scope.room_title = decodeURIComponent(room_title);
            try {
                window.cefQuery({
                    request: JSON.stringify({ type: 'RoomNickName', room_nick_name: $scope.room_title }),
                    onSuccess: function (response) { },
                    onFailure: function (error_code, error_message) { }
                });
            } catch (err) { }
        }
        $scope.$apply();
    });

    /* join ----------------------------------------------- */
    ws.$on('join', function (data) {
        if (data.result == 'success') {
            transformURIComponentArray(data.users, 'name', decodeURIComponent);
            $scope.users = data.users;

            var room_title = '';            
            findAndRemove($scope.users, 'name', $scope.MyName);
            $scope.roomNickName = getKeyValueIf($scope.users, 'room_nick_name', 'name', $scope.MyName);
            if ($scope.roomNickName == undefined || $scope.roomNickName.length == 0) {
                room_title = concatKeyValue($scope.users, 'name');
            }else{
                room_title = $scope.roomNickName;
            }
            if ($scope.users.length == 0) {
                room_title = room_title + '(비어있음)';
            } else {
                var totalCnt = $scope.users.length + 1;
                if (totalCnt > 2) {
                    room_title = room_title + '(' + totalCnt + ')';
                }
            }
            $scope.room_title = decodeURIComponent(room_title);
            try {
                window.cefQuery({
                    request: JSON.stringify({ type: 'RoomNickName', room_nick_name: $scope.room_title }),
                    onSuccess: function (response) { },
                    onFailure: function (error_code, error_message) { }
                });
            } catch (err) { }
        } else if (data.result == 'already') {
            try {
                window.cefQuery({
                    request: JSON.stringify({ type: 'Foreground' }),
                    onSuccess: function (response) { },
                    onFailure: function (error_code, error_message) { }
                });
            } catch (err) { }
            return;
        }else {
            $scope.setState('close', "채팅방을 찾을수 없습니다.");
        }
        $scope.$apply();
    });    

    $scope.send = function () {
        if ($scope.content != undefined && $scope.content.length > 0 && $scope.content.length < 1000) {
            var data = {
                room_name: encodeURIComponent($scope.RoomName),
                user_name: encodeURIComponent($scope.MyName),
                content: encodeURIComponent($scope.content)
            };
            ws.$emit('message', data);
            $scope.content = "";            
        } else {
            if ($scope.content == undefined || $scope.content.length == 0) {
                printAlertMessage("비어있는 메세지는 보낼수 없습니다.");
            }else {
                printAlertMessage("메세지 최대길이는 1000글자 입니다.");
            }
        }
    };

    $scope.more = function () {
        var data;
        if ($scope.messages == undefined || $scope.messages.length == 0) {
            data = {
                room_name: encodeURIComponent($scope.RoomName),
                start_seq: 0
            };
        } else {
            data = {
                room_name: encodeURIComponent($scope.RoomName),
                start_seq: $scope.messages[0].seq - 1
            };
        }
        ws.$emit('more', data);
    };

    ws.$on('message', function (data) {
        var isEnd;
        isEnd = isScrollEnd();
        if (data.result == 'success') {
            data.user_name= decodeURIComponent(data.user_name),
            data.content= decodeURIComponent(data.content)
            $scope.messages.push(data);
        } else {
        }
        if (data.user_name != $scope.MyName) {
            try {
                var reqData = {
                    type: 'MessageReceive',
                    data: {
                        window_type: 'room',
                        last_sender: encodeURIComponent(data.user_name),
                        last_message: encodeURIComponent(data.content),
                        room_name: encodeURIComponent($scope.RoomName)
                    }
                };
                window.cefQuery({
                    request: JSON.stringify(reqData),
                    onSuccess: function (response) { },
                    onFailure: function (error_code, error_message) { }
                });
            } catch (err) {}
        } else {
            $scope.seq();
        }
        $scope.$apply();

        if (isEnd) {
            scrollAdjust();
        } else {
            printNewMessage(data.content);
        }
    });

    var isFirst = true;
    ws.$on('message_s', function (data) {
        transformURIComponentArray(data.messages, 'user_name', decodeURIComponent);
        transformURIComponentArray(data.messages, 'content', decodeURIComponent);
        $scope.messages = data.messages.reverse().concat($scope.messages);
        $scope.calcMessageDate();

        var userCount = data.users.length;
        $.each($scope.messages, function (index, arrayValue) {
            $scope.messages[index].unread_count = userCount;
        });
        $.each(data.users, function (index, arrayValue) {
            var lseq = arrayValue['last_read_seq'];
            var sseq = arrayValue['start_read_seq'];
            if (decodeURIComponent(arrayValue['name']) == $scope.MyName) {
                $scope.startReadSeq = parseInt(sseq);
            }
            $.each($scope.messages, function (index1, arrayValue1) {
                if (lseq == 0 || lseq < sseq) lseq = sseq;
                if (parseInt($scope.messages[index1].seq) <= parseInt(lseq)) {
                    $scope.messages[index1].unread_count--;
                }
            });
        });

        $scope.$apply();
        $scope.seq();
        if (isFirst == true) {
            isFirst = false;
            scrollAdjust();
        }
    });

    $scope.calcMessageDate = function () {
        var len = $scope.messages.length - 1;
        var now;
        $.each($scope.messages, function (index, arrayValue) {
            if(len == index) 
                return false;
            var before_msg = $scope.messages[index];
            var next_msg = $scope.messages[index + 1];
            if (before_msg.time != undefined && next_msg.time != undefined) {
                var before = before_msg.time.substr(0, 10);
                var next = next_msg.time.substr(0, 10);
                if (before != next && now != next) {
                    now = next.toString();
                    $scope.messages.splice(index + 1, 0,{content: now, user_name: "DATE", seq: before_msg.seq});
                }
            }
        });
    };

    ws.$on('room', function (data) {
        var userCount = data.room_users.length;
        $.each($scope.messages, function (index, arrayValue) {
            $scope.messages[index].unread_count = userCount;
        });
        $.each(data.room_users, function (index, arrayValue) {
            var lseq = parseInt(arrayValue['last_read_seq']);
            var sseq = parseInt(arrayValue['start_read_seq']);
            if (decodeURIComponent(arrayValue['name']) == $scope.MyName) {
                $scope.startReadSeq = parseInt(sseq);
            }
            $.each($scope.messages, function (index1, arrayValue1) {
                if (lseq == 0 || lseq < sseq) lseq = sseq;
                if (parseInt($scope.messages[index1].seq) <= parseInt(lseq)){
                    $scope.messages[index1].unread_count--;
                }
            });
        });
        $scope.$apply();
    });

    ws.$on('users', function (data) {
        transformURIComponentArray(data, 'name', decodeURIComponent);
        findAndRemove(data, 'name', $scope.MyName);
        findAndRemoveArray(data, 'name', $scope.users);        
        $scope.friends = data;
        $scope.$apply();
    });

    $scope.invitePopup = function () {
        ws.$emit('users', { });
        inviteListOpen();
    };

    $scope.invite = function () {
        var invite_list = copyIf($scope.friends, 'is_checked', true);
        if (invite_list.length == 0)
            return;
        var data = {
            room_name: $scope.RoomName,
            invite_list: transformURIComponentArrayCopy(invite_list, 'name', encodeURIComponent)
        };
        ws.$emit('invite', data);
        inviteListClose();
    };

    $scope.selectedCount = function () {
        var invite_list = copyIf($scope.friends, 'is_checked', true);
        return invite_list.length;
    }

    ws.$on('invite', function (data) {
        if (data.result == 'success') {
            transformURIComponentArray(data.users, 'name', decodeURIComponent);
            $scope.users = data.users;
            findAndRemove($scope.users, 'name', $scope.MyName);
            
            var room_title = ''
            if ($scope.roomNickName == undefined || $scope.roomNickName == '') {
                // 닉네임을 사용하는게 아니라면 유저이름에서 가져온다.
                room_title = concatKeyValue($scope.users, 'name');
            } else {
                // 닉네임을 사용중이라면 그대로 사용한다.
                room_title = $scope.roomNickName;
            }
            if ($scope.users.length == 0) {
                room_title = room_title + ' (비어있음)';
            } else {
                var totalCnt = $scope.users.length + 1;
                if (totalCnt > 2) {
                    room_title = room_title + ' (' + totalCnt + ')';
                }
            }
            $scope.room_title = decodeURIComponent(room_title);
        }
        $scope.$apply();
    });

    $scope.leave = function () {
        var data = {
            room_name: encodeURIComponent($scope.RoomName),
            user_name: encodeURIComponent($scope.MyName),
        };
        ws.$emit('leave', data);
        window.open('', '_self').close();
    };

    $scope.close = function () {
        window.open('', '_self').close();
    };

    $scope.seq = function () {
        if ($scope.messages.length > 0) {
            var lastSeq = $scope.messages[$scope.messages.length - 1].seq;
            if (parseInt(lastSeq) > parseInt($scope.lastSendSeq)) {
                var data = {
                    room_name: encodeURIComponent($scope.RoomName),
                    user_name: encodeURIComponent($scope.MyName),
                    seq: lastSeq
                };
                ws.$emit('seq', data);
                $scope.lastSendSeq = lastSeq;
            }
        }
    };

    $scope.keyDown = function (event) {
        if (event.keyCode == 13) {
            if ($scope.content == undefined || $scope.content.length == 0) {
                event.preventDefault();
            }
        }
    };

    $scope.keyUp = function (event) {
        if (event.keyCode == 13)
        {
            if ($scope.content == undefined || $scope.content.length == 0) {
                event.preventDefault();
            } else if (event.ctrlKey == true || event.shiftKey == true) {
                $scope.content += '\r\n';
            } else {
                $scope.send();
            }
            return;
        }

        if (event.keyCode > 48 && event.keyCode < 58) {
            var ipcName;
            if(event.shiftKey == true){
                ipcName = "KB_IPC_MANAGER";
            } else {
                ipcName = "YSARANG_IPC_MANAGER";
            }
            if (event.ctrlKey == true) {
                var reqData = {
                    type: 'Keyboard',
                    data: {
                        name: ipcName,
                        altKey: event.altKey,
                        ctrlKey: event.ctrlKey,
                        shiftKey: event.shiftKey,
                        keyCode: event.keyCode,
                    }
                };
                try {
                window.cefQuery({
                    request: JSON.stringify(reqData),
                    onSuccess: function (response) {
                        if ($scope.content == undefined || $scope.content.length == 0) {
                            $scope.content = response;
                        } else {
                            $scope.content += response;
                        }
                        $scope.$apply();
                    },
                    onFailure: function (error_code, error_message) {
                        $scope.content = error_message;
                        $scope.$apply();
                    }
                });
                }catch(err) {}
            }
        }
    }

    $scope.setState = function (type, message, messageEx) {

        if ($('.modal-header').is(':visible'))
            return;

        var modalInstance = $modal.open({
            animation: true,
            templateUrl: 'state.html',
            controller: 'myStateModalController',
            backdrop: 'static',
            size: 'sm',
            resolve: {
                state: function () {
                    var data = {
                        type: type,
                        message: message,
                        messageEx: messageEx
                    }
                    return data;
                }
            }
        });

        modalInstance.result.then(function () {
            window.open('', '_self').close();
        }, function () {
        });
    };
});

myChat.controller('myStateModalController', function ($scope, $modalInstance, state) {
    $scope.state = state;
    $scope.close = function () {
        $modalInstance.close();
    };
});
