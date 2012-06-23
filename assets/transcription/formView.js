function getParameter(paramName, defaultValue) {
    var searchString = window.location.search.substring(1);
    searchString = searchString ? searchString : window.location.hash.substring(2);
    var params = searchString.split('&');
    for (var i = 0; i < params.length; i++) {
        var val = params[i].split('=');
        if (val[0] === paramName) {
            return decodeURI(val[1]);
        }
    }
    return defaultValue;
}

function computeTextLocation(field) {
    var minX = 999999;
    var avgY = 0;
    $(field.segments).each(

    function(segment_idx) {
        var segment = this;
        if (segment.quad) {
            var quad = segment.quad;
            avgY += (quad[0][1] + quad[1][1] + quad[2][1] + quad[3][1]) / 4;
            if (quad[0][0] < minX) {
                minX = quad[0][0];
            }
        }
    });
    avgY /= field.segments.length;
    return {
        x: Math.abs(minX - 30),
        y: avgY
    };
}
jQuery(function($) {
    //Saving behavior
    $('.save').attr("disabled", true).text('saved');
    $("form").submit(function(e) {
        e.preventDefault();
        $('.save').text('saving...');
        console.log($('.modified').serialize());
        $.post("/save_transcription/", $('.modified').serialize(), function() {
            $('.save').attr("disabled", true).text('saved');
            $('.modified').removeClass('modified');
        });
    });

    var selectedSegment = getParameter('segment');
    var $canvas = $("canvas").jCanvas();
    $(".main-image").attr('src', getParameter('imageUrl', "default.png"));
    $(".main-image").load(function() {
        $canvas.attr('height', $(this).height());
        $canvas.attr('width', $(this).width());

        $.getJSON(getParameter('jsonOutputUrl', ""), function(form) {
            var $bar = $('.bar');
            var progress = 10;
            var numFields = form.fields.length;
            $bar.css('width', '10%');
            $(form.fields).each(
            function(field_idx) {
                progress += 90/numFields;
                $bar.css('width', progress + '%');
                var field = this;
                $(field.segments).each(
                function(segment_idx) {
                    var segment = this;
                    var isSelected = (selectedSegment == field.name + '_image_' + segment_idx);
                    if (segment.quad) {
                        var quad = segment.quad;
                        $canvas.addLayer({
                            method: 'drawLine',
                            strokeStyle: (isSelected ? "#fff200" : "#ffb700"),
                            strokeWidth: 4,
                            x1: quad[0][0],
                            y1: quad[0][1],
                            x2: quad[1][0],
                            y2: quad[1][1],
                            x3: quad[2][0],
                            y3: quad[2][1],
                            x4: quad[3][0],
                            y4: quad[3][1],
                            closed: true,
                            radius: 100,

                            mouseover: function(layer) {
                                var segmentQuad = $(this);
                                var originalStrokeStyle = layer.strokeStyle;
                                segmentQuad.animateLayer(layer, {
                                    strokeStyle: "#ff2600"
                                }, 300, function() {
                                    segmentQuad.animateLayer(layer, {
                                        strokeStyle: originalStrokeStyle
                                    }, 300);
                                });
                            },

                            click: function(layer) {
                                var segmentQuad = $(this);
                                segmentQuad.animateLayer(layer, {
                                    strokeStyle: "#00ff09"
                                }, 50);
                                if (Android !== 'undefined') {
                                    Android.launchCollect(field.name, segment_idx, field_idx);
                                }
                                else {
                                    var $body = $('.modal-body');
                                    $body.empty();
                                    $(field.segments).each(

                                    function(segment_idx) {
                                        $body.append(
                                        $('<img>').attr('src', 'img0/segments/' + field.name + '_image_' + segment_idx + '.jpg'));
                                    });
                                    var $input = $('<input>');
                                    $input.attr('name', field.name);
                                    $input.val(field.transcription || field.value);
                                    $input.keydown(function() {
                                        $(this).addClass('modified');
                                        $('.save').attr("disabled", false).text('save');
                                    });
                                    $body.append($input);
/*
                                    var saveBtn = $('<a>').addClass("btn").addClass("btn-primary").addClass("save");
                                    saveBtn.click(function(){
                                        var input;
                                        input.val();
                                    });
                                    $(".save").replaceWith(saveBtn);
                                    */
                                    //window.location = "fieldView.html";
                                    $('#myModal').modal('show');
                                }
                            }
                        });
                        $(segment.items).each(

                        function(item_idx) {
                            var item = this;
                            console.log(item);
                            $canvas.addLayer({
                                method: "drawRect",
                                strokeStyle: item.classification ? "#00ff44" : "#ff00ff",
                                strokeWidth: 3,
                                opacity: .4,
                                fromCenter: true,
                                name: "myBox",
                                group: "myBoxes",
                                x: item.absolute_location[0],
                                y: item.absolute_location[1],
                                width: 5,
                                height: 5
                            });
                        });
                    }
                });
                var textLocation = computeTextLocation(field);
                $canvas.addLayer({
                    method: "drawText",
                    fillStyle: "#9cf",
                    strokeStyle: "#25a",
                    opacity: .7,
                    strokeWidth: 4,
                    x: textLocation.x,
                    y: textLocation.y,
                    font: "12pt Verdana, sans-serif",
                    text: field.transcription || field.value
                });
            });
            $canvas.click();

        });

    });

    window.onbeforeunload = function() {
        if ($('.modified').length > 0) {
            return "If you navigate away from this page you will loose unsaved changes.";
        }
    };

});