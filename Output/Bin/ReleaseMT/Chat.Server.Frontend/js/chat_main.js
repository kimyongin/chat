'use strict';


$(window).resize(function () {
    layoutInit();
});

function mainTabChange(obj) {
    var $obj = $(obj).first();
    $obj.siblings('a').children().children().removeClass('selected');
    $obj.children().children().addClass('selected');
}

function layoutInit() {
    var _height = $(window).height() - $('.header').outerHeight() - $('.list-filter').outerHeight();
    var $obj1 = $('.group-user-detail');
    if ($obj1.is(':visible')) {
        _height = _height - $obj1.outerHeight();
    }
    var $obj2 = $('.main-banner-wrap');
    if ($obj2.is(':visible')) {
        _height = _height - $obj2.outerHeight();
    }
    var maxHeight = String(_height) + 'px';
    $('.list-wrapper').css({
        overflowY:'auto', 
        maxHeight:_height
    });
}

function groupChatShow(isShow) {
    if (isShow == true)
    {
        $('#tabUser').trigger('click');
        $('[name=userCheck]').show();
        $('#groupUserDetail').show();
        $('#mainBanner').hide();        
    }
    else
    {
        var $obj = $('[name=userCheck]');
        $obj.each(function (i) {
            var $this = $obj.eq(i);
            if ($this.is(':checked')) {
                $this.trigger('click');
            }
        });
        $obj.hide();
        $('#groupUserDetail').hide();
        $('#mainBanner').show();
    }
    layoutInit();
}

function triggerClick(event, _this, _name) {
    var $this = $(_this);
    var $obj = $('[name=' + _name + ']');

    if ($obj.is(':visible')) {
        $this.find($obj).trigger('click');
    }
}

var myChat = angular.module('myChatMain', ['ngRoute', 'ngWebsocket', 'myBanner', 'ui.bootstrap', 'ui.bootstrap.contextMenu']);

myChat.config(function ($locationProvider) {
    $locationProvider.html5Mode({
        enabled: true,
        requireBase: false
    });    
});

myChat.config(function (bannerServiceProvider) {
    bannerServiceProvider.setClientID("");
});

myChat.config(['$routeProvider', function ($routeProvider) {
    $routeProvider.
    when('/chat_main/friend', { templateUrl: 'chat_main_friend.html', controller: 'myChatFriendController' }).
    when('/chat_main/room', { templateUrl: 'chat_main_room.html', controller: 'myChatRoomController' }).    
    otherwise({ redirectTo: '/chat_main/friend' });
}]);

myChat.controller('myChatMainController', function ($scope, $http, $location, $route, $websocket, $modal, $log, bannerService) {

    var search = $location.search();
    var bannerServiceUrl = search.banner_service;
    $scope.MyDecName = search.user_name;
    $scope.MyName = encodeURIComponent(search.user_name);

    $scope.menuOptions = [];
    $scope.menuOptions.push(['그룹대화', function ($itemScope) { groupChatShow(true); }]);
    if (search.is_auto == false || search.is_auto == undefined) {
        $scope.menuOptions.push(null);
        $scope.menuOptions.push(['로그아웃', function ($itemScope) { $scope.logout(); }]);
    }
    $scope.menuOptions.push(null);
    $scope.menuOptions.push(['제품정보', function ($itemScope) { $scope.productInfo(); }]);

    bannerService.setServiceUrl(bannerServiceUrl);
    bannerService.load();
    $scope.$on("bannerState", function (event, args) {
        if (args == true) {
            $scope.bannerShow = true;
        }else {
            $scope.bannerShow = false;
            bannerService.setServiceUrl("/banner/list.json");
            bannerService.load();
        }
        layoutInit();
    });
    
    $scope.users = [];
    $scope.rooms = [];

    $scope.$watch('users', function (newValue, oldValue) {
        $scope.countCheckedUser = countIf($scope.users, 'is_checked', true);
    }, true);

    var wsUrl = "ws://" + $location.host() + ":" + $location.port() + "/chat/ws";
    $scope.ws = $websocket.$new({
        url: wsUrl,
        reconnect: true,
        reconnectInterval: 2000
    });

    /* open, close ----------------------------------------------- */
    $scope.ws.$on('$open', function () {
        $scope.regist();
        $scope.$apply();
        $('#modalReconnectBtn').trigger('click');
    });
    $scope.ws.$on('$close', function () {
        $scope.setState('reconnect', "서버와의 연결이 해제 되었습니다.", "재접속을 시도 합니다.");
        $scope.users = [];
        $scope.rooms = [];
        $scope.$apply();
    });

    $scope.logout = function () {
        $location.path("/chat_login");
        window.location = $location.path();
    };

    /* regist ----------------------------------------------- */
    $scope.regist = function () {
        var data = {
            user_name: $scope.MyName
        }
        $scope.ws.$emit('regist', data);
    };
    $scope.ws.$on('regist', function (data) {
        if (data.result == 'success') {
        } else {
            $scope.setState('close', "로그인이 해제 되었습니다.", "다시 로그인을 해주세요");
        }
        $scope.$apply();
    });
    
    /* unregist ----------------------------------------------- */
    $scope.unregist = function () {
        $scope.users = [];
        $scope.rooms = [];
        var data = {
            user_name: $scope.MyName
        }
        $scope.ws.$emit('unregist', data);
    }
    $scope.ws.$on('unregist', function (data) {
        $scope.fail();
        $scope.$apply();
    });

    /* users ----------------------------------------------- */
    $scope.ws.$on('users', function (data) {
        findAndRemove(data, 'name', $scope.MyName);
        transformURIComponentArray(data, 'name', decodeURIComponent);
        var temp = $scope.users;        
        $scope.users = data;
        copyPropArray($scope.users, temp, 'name', 'is_checked'); //기존의 체크값을 유지한다.
        $scope.$apply();
        if ($('#groupUserDetail').is(':visible')) {
            $('[name=userCheck]').show();
        }
        layoutInit();
    });

    /* rooms ----------------------------------------------- */
    $scope.unreadCount = function () {
        var totalUnreadCount = 0;
        $.each($scope.rooms, function (index, obj) {
            totalUnreadCount = totalUnreadCount + obj.unread_count;
        });
        return totalUnreadCount;
    }
    $scope.ws.$on('roomname', function (data) {
        $.each($scope.rooms, function (index, room) {
            if (room.room_name == data.room_name) {
                room.room_nick_name = decodeURIComponent(data.room_nick_name);
            }
        });
        $scope.$apply();
    });
    $scope.ws.$on('room', function (data) {
        findAndRemove($scope.rooms, 'room_name', data.room_name);
        var sseq = parseInt(getKeyValueIf(data.room_users, 'start_read_seq', 'name', $scope.MyName));
        var lseq = parseInt(getKeyValueIf(data.room_users, 'last_read_seq', 'name', $scope.MyName));
        if (lseq == 0 || lseq < sseq) lseq = sseq;
        if (data.last_sender == $scope.MyName && lseq + 1 == data.last_message_seq) {
            lseq = lseq + 1;
        }
        var room_nick_name = getKeyValueIf(data.room_users, 'room_nick_name', 'name', $scope.MyName);
        findAndRemove(data.room_users, 'name', $scope.MyName);
        if (room_nick_name == undefined || room_nick_name.length == 0) {
            room_nick_name = concatKeyValue(data.room_users, 'name');
        }
        if (data.room_users.length == 0) {
            room_nick_name = room_nick_name + '(비어있음)';
        }

        var roomData = {
            room_name: decodeURIComponent(data.room_name),
            room_nick_name: decodeURIComponent(room_nick_name),
            room_users: transformURIComponentArray(data.room_users, 'name', decodeURIComponent),
            last_sender: decodeURIComponent(data.last_sender),
            last_message: decodeURIComponent(data.last_message),
            last_time: data.last_time,
            unread_count: data.last_message_seq - lseq
        }
        $scope.rooms.push(roomData);               
        $scope.$apply();
        layoutInit();
    });
    $scope.ws.$on('room_s', function (datas) {
        $scope.rooms = [];
        $.each(datas, function (index, data)
        {
            var sseq = parseInt(getKeyValueIf(data.room_users, 'start_read_seq', 'name', $scope.MyName));
            var lseq = getKeyValueIf(data.room_users, 'last_read_seq', 'name', $scope.MyName);
            if (lseq == 0 || lseq < sseq) lseq = sseq;
            var room_nick_name = getKeyValueIf(data.room_users, 'room_nick_name', 'name', $scope.MyName);
            findAndRemove(data.room_users, 'name', $scope.MyName);            
            if (room_nick_name == undefined || room_nick_name.length == 0) {
                room_nick_name = concatKeyValue(data.room_users, 'name');
            }
            if (data.room_users.length == 0) {
                room_nick_name = room_nick_name + '(비어있음)';
            }
            var roomData = {
                room_name: decodeURIComponent(data.room_name),
                room_nick_name: decodeURIComponent(room_nick_name),
                room_users: transformURIComponentArray(data.room_users, 'name', decodeURIComponent),
                last_sender: decodeURIComponent(data.last_sender),
                last_message: decodeURIComponent(data.last_message),
                last_time: data.last_time,
                unread_count: data.last_message_seq - lseq
            }
            $scope.rooms.push(roomData);
        });
        $scope.$apply();
        layoutInit();
    });
    
    /* alarm ----------------------------------------------- */
    $scope.ws.$on('alarm', function (data) {        
        if (data.last_sender != $scope.MyName) {
            try {
                var reqData = {
                    type: 'MessageReceive',
                    data: data
                };
                reqData.data.window_type = 'main';
                window.cefQuery({
                    request: JSON.stringify(reqData),
                    onSuccess: function (response) { },
                    onFailure: function (error_code, error_message) { }
                });
            } catch (err) { }
        }
    });

    /* make ----------------------------------------------- */
    $scope.ws.$on('make', function (data) {
        if (data.result == 'success') {
            var roomUrl = $location.protocol() + "://" + $location.host() + ":" + $location.port() + data.path + "?room_name=" + data.room_name + "&user_name=" + $scope.MyName;
            window.open(roomUrl, "", "width=400,height=600,toolbar=yes,menubar=no,scrollbars=no,resizable=no,copyhistory=no");
        }
    });
    $scope.make = function (data) {
        var data = {
            user_name: $scope.MyName,
            room_name: data.room_name
        };
        $scope.ws.$emit('make', data);
    }

    $scope.setSelected = function () {
        // console.log("show", arguments, this);
        if ($scope.lastSelected) {
            $scope.lastSelected.selected = '';
        }
        this.selected = 'selected';
        $scope.lastSelected = this;
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
            $scope.logout();
        }, function () {
        });
    };

    $scope.productInfo = function () {

        if ($('.modal-header').is(':visible'))
            return;

        var modalInstance = $modal.open({
            animation: true,
            templateUrl: '/template/product_info.html',
            controller: 'myProductInfoController',
            size: 'sm',
            backdrop: 'static',
            resolve: {
                info: function () {
                    var data = {
                        version: "1.0",
                        server: $location.$$host
                    }
                    return data;
                }
            }
        });

        modalInstance.result.then(function () {
        }, function () {
        });
    };
});

myChat.controller('myStateModalController', function ($scope, $modalInstance, state) {
    $scope.state = state;
    $scope.close = function () {
        $modalInstance.close();        
    };
    $scope.dismiss = function () {
        $modalInstance.dismiss();
    }
});

myChat.controller('myChatFriendController', function ($scope, $http, $location, $route, $websocket) {

    $scope.$on('$routeChangeSuccess', function () {
        layoutInit();
    });

    $scope.make = function (friendName) {
        if ($('#groupUserDetail').is(':visible')) {
            return;
        }

        var invite_list = copyIf($scope.users, 'name', friendName);
        var data = {
            user_name: $scope.MyName,
            invite_list: transformURIComponentArrayCopy(invite_list, 'name', encodeURIComponent)
        };
        $scope.ws.$emit('make', data);
    };

    $scope.make_group = function () {
        $scope.before = Date.now();

        var invite_list = copyIf($scope.users, 'is_checked', true);
        if (invite_list.length == 0)
            return;
        var data = {
            user_name: $scope.MyName,
            invite_list: transformURIComponentArrayCopy(invite_list, 'name', encodeURIComponent)
        };
        $scope.ws.$emit('make', data);
    };

    $scope.selectedCount = function () {
        var invite_list = copyIf($scope.users, 'is_checked', true);
        return invite_list.length;
    }
});

myChat.controller('myChatRoomController', function ($scope, $http, $location, $route, $websocket, $modal, $log) {

    $scope.$on('$routeChangeSuccess', function () {
        layoutInit();
    });

    $scope.make_room = function (roomUsers) {
        var data = {
            user_name: $scope.MyName,
            invite_list: transformURIComponentArrayCopy(roomUsers, 'name', encodeURIComponent)
        };
        $scope.ws.$emit('make', data);
    };

    $scope.leave = function (roomName) {
        var data = {
            room_name: encodeURIComponent(roomName),
            user_name: $scope.MyName,
        };
        $scope.ws.$emit('leave', data);
    };

    $scope.menuOptionsForRoom = [
    [
        '나가기',
        function ($itemScope) {
            $scope.leave($itemScope.room.room_name);
        }
    ],    
    [
        '이름변경',
        function ($itemScope) {
            $scope.nameChange($itemScope.room.room_name, $itemScope.room.room_nick_name);
        }
    ]];

    $scope.nameChange = function (roomName, roomNickName) {

        if ($('.modal-header').is(':visible'))
            return;

        var modalInstance = $modal.open({
            animation: true,
            templateUrl: '/template/room_name.html',
            controller: 'myRoomNameModalController',
            size: 'sm',
            backdrop: 'static',
            resolve: {
                roomNickName: function () {
                    return roomNickName;
                }
            }
        });

        modalInstance.result.then(function (newRoomNickName) {
            var data = {
                user_name: $scope.MyName,
                room_name: roomName,
                room_nick_name: encodeURIComponent(newRoomNickName)
            };
            $scope.ws.$emit('roomname', data);

        }, function () {

        });
    };
});

myChat.controller('myRoomNameModalController', function ($scope, $modalInstance, roomNickName) {
    $scope.roomNickName = roomNickName;

    $scope.ok = function () {
        if ($scope.roomNickName == undefined || $scope.roomNickName.length <= 0 || $scope.roomNickName.length > 20) {
            return;
        }
        $modalInstance.close($scope.roomNickName);
    };
    $scope.cancel = function () {
        $modalInstance.dismiss('cancel');
    };
});

myChat.controller('myProductInfoController', function ($scope, $modalInstance, info) {
    $scope.version = info.version;
    $scope.server = info.server;
    $scope.ok = function () {
        $modalInstance.close($scope.roomNickName);
    };
    $scope.cancel = function () {
        $modalInstance.dismiss('cancel');
    };
});

function make(data) {
    var scope = angular.element(document.getElementById("main")).scope();
    scope.$apply(function () {
        scope.make(data);
    });
}