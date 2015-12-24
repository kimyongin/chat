'use strict';

var myMonitor = angular.module('myMonitor', ['ngWebsocket']);
myMonitor.controller('myMonitorController', function ($scope, $http, $location, $websocket) {

    $scope.logs = []
    $scope.sessions = []

    var sessionUrl = $location.absUrl() + '/sessions';
    $http.get(sessionUrl).
        success(function (data, status, headers, config) {
            $scope.sessions = data.Sessions;
            $scope.$apply();
        }).
        error(function (data, status, headers, config) {
        });

    var wsUrl = $location.absUrl().replace('http', 'ws') + '/ws';    
    var ws = $websocket.$new(wsUrl);

    ws.$on('$open', function () {
        $scope.state = 'open';
        $scope.$apply(); 
    });

    ws.$on('Log', function (data) {
        $scope.logs.push(data);
        $scope.$apply(); 
    });

    ws.$on('Sessions', function (data) {
        $scope.sessions = data.Sessions;
        $scope.$apply();
    });

    ws.$on('$close', function () {
        $scope.state = 'close';
        $scope.$apply(); 
    });
});