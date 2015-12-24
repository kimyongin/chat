angular.module("myBanner", [])
.provider("bannerService", function () {

    var serviceUrl = '';
    var clientID = '';
    var adDatas = [];
    var currentIndex = 0;
    var tryCount = 0;

    return {
        setClientID: function (id) {
            if (angular.isDefined(id)) {
                clientID = id;
                return this;
            } else {
                return clientID;
            }
        },
        $get: function ($http, $rootScope) {
            return {
                setServiceUrl: function (url) {
                    if (angular.isDefined(url)) {
                        serviceUrl = url;
                        return this;
                    } else {
                        return serviceUrl;
                    }
                },
                load: function(){
                    $http.get(serviceUrl).success(function (data, status, headers, config) {
                        $.each(data, function (index, obj) {
                            adDatas.push({
                                url: obj.fields.adLinkUrl,
                                img: obj.fields.adImageUrl,
                                width: obj.fields.width,
                                height: obj.fields.height,
                                background: obj.fields.background
                            })
                        });
                        currentIndex = 0;                        
                        $rootScope.$broadcast("bannerEvent", adDatas[currentIndex++ % adDatas.length]);
                    }).error(function (data, status, headers, config) {
                        if (tryCount == 0) {
                            tryCount++;
                            $rootScope.$broadcast("bannerState", false);                            
                        }
                    });
                },
                next: function () {
                    $rootScope.$broadcast("bannerEvent", adDatas[currentIndex++ % adDatas.length]);
                },
                prev: function () {
                    $rootScope.$broadcast("bannerEvent", adDatas[currentIndex-- % adDatas.length]);
                }
            }
        }
    }
})
.directive("bannerBox", function () {
    return {
        restrict: "EA",
        templateUrl: "/template/banner.html"
    }
})
.controller("bannerCtrl", function ($scope, $timeout, $rootScope, bannerService) {
    $scope.bannerItem = {};
    
    var isFirst = true;
    var timer;
    $scope.$on("bannerEvent", function (event, args) {
        $scope.bannerItem.url = args.url;
        $scope.bannerItem.img = args.img;
        $scope.bannerItem.width = args.width;
        $scope.bannerItem.height = args.height;
        $scope.bannerItem.background = '#' + args.background;        
        timer = $timeout(function () {
            bannerService.next();
        }, 5000);
        if (isFirst == true) {
            $timeout(function () {
                $rootScope.$broadcast("bannerState", true);
            }, 1000);
            isFirst = false;
        }
    });
    $scope.prevBanner = function () {
        $timeout.cancel(timer);
        bannerService.prev();
    }
    $scope.nextBanner = function () {
        $timeout.cancel(timer);
        bannerService.next();
    }
})