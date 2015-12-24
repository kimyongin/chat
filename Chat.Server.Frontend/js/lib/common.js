// JavaScript source code

function removeByIndex(arrayName, arrayIndex) {
    arrayName.splice(arrayIndex, 1);
}

function findAndRemove(array, key, value) {
    $.each(array, function (index, arrayValue) {
        if (arrayValue[key] == value) {
            array.splice(index, 1);
            return false;
        }
    });
}

function findAndRemoveArray(array, key, searchArray) {
    $.each(searchArray, function (index, arrayValue) {
        return findAndRemove(array, key, arrayValue[key])
    });
}

function copyIf(array, key, value) {
    var copy_result = [];
    $.each(array, function (index, arrayValue) {
        if (arrayValue[key] == value) {
            copy_result.push(arrayValue);
        }
    });
    return copy_result;
}

function copyProp(destArray, matchKey, matchValue, valueKey, valueValue) {
    $.each(destArray, function (destIndex, destArrayValue) {
        if (destArrayValue[matchKey] == matchValue) {
            destArrayValue[valueKey] = valueValue;
            return false;
        };
    });
}

function copyPropArray(destArray, sourceArray, matchKey, valueKey) {
    $.each(sourceArray, function (sourceIndex, sourceArrayValue) {
        copyProp(destArray, matchKey, sourceArrayValue[matchKey], valueKey, sourceArrayValue[valueKey]);
    });
}

function copyIfEx(array, key, value, func) {
    var copy_result = [];
    $.each(array, function (index, arrayValue) {
        if (arrayValue[key] == value) {
            copy_result.push(func(arrayValue));
        }
    });
    return copy_result;
}

function countIf(array, key, value) {
    var count = 0;
    $.each(array, function (index, arrayValue) {
        if (arrayValue[key] == value) {
            count++;
        }
    });
    return count;
}

function concatKeyValue(array, key) {
    var concat_result = '';
    $.each(array, function (index, arrayValue) {
        if (index > 0) {
            concat_result = concat_result.concat(",");
        }
        concat_result = concat_result.concat(arrayValue[key]);
    });
    return concat_result;
};

function getKeyValueIf(array, key, ifkey, ifvalue) {
    var value = null;
    $.each(array, function (index, arrayValue) {
        if (arrayValue[ifkey] == ifvalue) {
            value = arrayValue[key];
            return false;
        }
    });
    return value;
};

function transformURIComponentArray(array, key, func) {
    $.each(array, function (index, arrayValue) {
        arrayValue[key] = func(arrayValue[key]);
    });
    return array;
};

function transformURIComponentArrayCopy(array, key, func) {
    var resultArray = [];
    $.each(array, function (index, arrayValue) {
        var copyValue = JSON.parse(JSON.stringify(arrayValue));
        copyValue[key] = func(copyValue[key]);
        resultArray.push(copyValue);
    });
    return resultArray;
};

function checkClick(event) {
    event.stopPropagation();
}

function modalShow(func) {
    if ($('.common-modal').is(':visible')) {
        return;
    }

    var _html = '<div class="common-modal"></div>';
    $('body').append(_html);
    var $modal = $('.common-modal');
    $modal.css({
        display: 'none',
        zIndex: 50,
        position: 'fixed',
        width: '100%',
        height: '100%',
        left: '0',
        top: '0',
        backgroundColor: '#000',
        opacity: '0.6'
    });

    $modal.fadeIn(200);
    if (!func || typeof (func) == 'undefined') {
        return;
    }
    $modal.bind('click', function () {
        func();
    });
}

function modalClose() {
    var $modal = $('.common-modal');
    $modal.remove();
}
