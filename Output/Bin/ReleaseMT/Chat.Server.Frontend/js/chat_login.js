'use strict';

angular.module('myChatLogin.directive', [])
.directive('ngEnter', function () {
    return function (scope, element, attrs) {
        element.bind("keydown keypress", function (event) {
            if (event.which === 13) {
                scope.$apply(function () {
                    scope.$eval(attrs.ngEnter);
                });

                event.preventDefault();
            }
        });
    };
});

var myChat = angular.module('myChatLogin', ['ngRoute', 'myChatLogin.directive']);

myChat.config(function ($locationProvider) {
    $locationProvider.html5Mode({
        enabled: true,
        requireBase: false
    });
});

myChat.controller('myChatLoginController', function ($scope, $http, $location, $route) {
    var search = $location.search();
    $scope.MyName = search.user_name;
    if ($scope.MyName != undefined && $scope.MyName.length > 0) {
        $location.path("/chat_main?user_name=" + $scope.MyName + "&is_auto=" + true + "&banner_service=" + search.banner_service);
        window.location = $location.path();
    }
    $scope.login = function () {
        if ($scope.MyName == undefined || $scope.MyName.length == 0)
            return;
        $location.path("/chat_main?user_name=" + $scope.MyName + "&banner_service=" + search.banner_service);
        window.location = $location.path();
    }
});