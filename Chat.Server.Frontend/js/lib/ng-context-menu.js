angular.module('ui.bootstrap.contextMenu', [])

.directive('contextMenu', ["$parse", function ($parse) {
    var renderContextMenu = function ($scope, event, options, model, attrs) {
        if (!$) { var $ = angular.element; }
        $(attrs.contextMenuSelected).addClass('selected');
        $(event.currentTarget).addClass('context');
        var $contextMenu = $('<div>');
        $contextMenu.addClass('dropdown clearfix');
        var $ul = $('<ul>');
        $ul.addClass('dropdown-menu');
        $ul.attr({ 'role': 'menu' });

        var width = Math.max(
            document.body.scrollWidth, document.documentElement.scrollWidth,
            document.body.offsetWidth, document.documentElement.offsetWidth,
            document.body.clientWidth, document.documentElement.clientWidth
        );
        var widthMore = 0;
        if (event.pageX + parseInt(attrs.contextMenuWidth) > width)
            widthMore = parseInt(attrs.contextMenuWidth);

        $ul.css({
            display: 'block',
            position: 'absolute',
            left: (event.pageX - widthMore) + 'px',
            top: event.pageY + 'px'
        });
        angular.forEach(options, function (item, i) {
            var $li = $('<li>');
            if (item === null) {
                $li.addClass('divider');
            } else {
                var $a = $('<a>');
                $a.attr({ tabindex: '-1', href: '#' });
                var text = typeof item[0] == 'string' ? item[0] : item[0].call($scope, $scope, event, model);
                $a.text(text);
                $li.append($a);
                var enabled = angular.isDefined(item[2]) ? item[2].call($scope, $scope, event, text, model) : true;
                if (enabled) {
                    $li.on('click', function ($event) {
                        $event.preventDefault();
                        $scope.$apply(function () {
                            $(event.currentTarget).removeClass('context');
                            $contextMenu.remove();
                            item[1].call($scope, $scope, event, model);
                        });
                    });
                } else {
                    $li.on('click', function ($event) {
                        $event.preventDefault();
                    });
                    $li.addClass('disabled');
                }
            }
            $ul.append($li);
        });
        $contextMenu.append($ul);
        var height = Math.max(
            document.body.scrollHeight, document.documentElement.scrollHeight,
            document.body.offsetHeight, document.documentElement.offsetHeight,
            document.body.clientHeight, document.documentElement.clientHeight
        );
        $contextMenu.css({
            width: '100%',
            height: (height - 1) + 'px',
            position: 'absolute',
            top: 0,
            left: 0,
            zIndex: 9999
        });
        $(document).find('body').append($contextMenu);
        $contextMenu.on("mousedown", function (e) {
            if ($(e.target).hasClass('dropdown')) {
                $(event.currentTarget).removeClass('context');
                $contextMenu.remove();
                $(attrs.contextMenuSelected).removeClass('selected');
            }
        }).on('contextmenu', function (event) {
            $(event.currentTarget).removeClass('context');
            event.preventDefault();
            $contextMenu.remove();
            $(attrs.contextMenuSelected).removeClass('selected');
        });
    };
    return function ($scope, element, attrs) {
        if (attrs.contextMenuClick == "right") {
            element.on('contextmenu', function (event) {
                event.stopPropagation();
                $scope.$apply(function () {
                    event.preventDefault();
                    var options = $scope.$eval(attrs.contextMenu);
                    var model = $scope.$eval(attrs.model);
                    if (options instanceof Array) {
                        if (options.length === 0) { return; }
                        renderContextMenu($scope, event, options, model, attrs);
                    } else {
                        throw '"' + attrs.contextMenu + '" not an array';
                    }
                });
            });
        }
        else if (attrs.contextMenuClick == "left") {
            element.on('click', function (event) {
                event.stopPropagation();
                $scope.$apply(function () {
                    event.preventDefault();
                    var options = $scope.$eval(attrs.contextMenu);
                    var model = $scope.$eval(attrs.model);
                    if (options instanceof Array) {
                        if (options.length === 0) { return; }
                        renderContextMenu($scope, event, options, model, attrs);
                    } else {
                        throw '"' + attrs.contextMenu + '" not an array';
                    }
                });
            });
        }
    };
}]);