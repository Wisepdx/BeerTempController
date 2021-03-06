

var csvArray = [];
var histArray = [];
var csvString;
var currentBatchID;
var currentBatchSize;
var currentBatchName;
var currentCsvArray;
var lastUpdate;
var mailboxURL = "";
var chartCurrentTemp = "null";

//get current page url and store in variable
var pathname = window.location.pathname;


function loadFile(path, destinationArray, callbackFunction) {
    $.ajax({
        method: "GET",
        url: path,
        dataType: "text",
        success: function(data) {
            debug("Loaded: " + path);
            destinationArray = parseCsvSimple(data, 1);
            callbackFunction(destinationArray);
        },
        fail: function() {
            debug("Error loading data");
        }
    });
}

function currentDataLoaded(data) {
    //current batch parts into variables
    currentBatchID = data[0][1];
    currentBatchName = data[0][2];
    currentBatchSize = data[0][3];

    if (pathname == "index") {
        //current batch vars into HTML 
        $("#batchIDVariable").html("<b>Batch ID: </b><span>" + currentBatchID + "</span>");
        $("#batchNameVariable").html("<b>Batch Name: </b><span>" + currentBatchName + "</span>");
        $("#batchSizeVariable").html("<b>Batch Size: </b><span>" + currentBatchSize + " Gallons</span>");
    } else {
        //not necessary to load
    }

    //create batch dropdown links for menu
    var batchMenuNumber = currentBatchID;
    var batchMenuHTML = ""
    for (i = 1; i <= currentBatchID; i++) {
        batchMenuHTML += '<li><a href="' + i + '.html">Batch #' + i + '</a></li>';
    }
    $("#batchDropdown").html(batchMenuHTML);
    
    //if batch is a historical page set batch number to pathname
    if ((pathname !== "index") && (pathname !== "changeSpecs")){
        currentBatchID = pathname;
    }
    
    if (pathname == "index") {
        loadFile("data/" + currentBatchID + ".csv", histArray, histDataLoaded);
    } else if (pathname == "changeSpecs") {
        //dont load a history chart
    } else {
        loadFile("data/" + pathname + ".csv", histArray, histDataLoaded);
    }
}

function histDataLoaded(data) {

    createHistoryChart(data);
}
$(document).ready(function() {
    //remove bs in front and .html
    pathname = pathname.replace(/\/\S*\//, ""); //removes all until last / in url path
    pathname = pathname.replace(/\Whtml/, ""); //removes .html in url path
    
    //Load current and parse
    loadFile("data/current.csv", csvArray, currentDataLoaded);
    
    bindDebug("D", ToggleDebug, null);
    
    $("#loadLink").click(function() {
        loadFile($("#loadBox").val());
    });

    setupLiveCharts();
    //updateLiveCharts();
});

// create the actual chart after the data is loaded
function createHistoryChart(data) {

    $('#container').highcharts('StockChart', {
        //title: {
        //    text: 'Batch #' + getColumn(data,1)[0] + " History",
        //},
        subtitle: {
            text: ''
        },
        xAxis: {
            gapGridLineWidth: 0
        },
        yAxis: {
            gridLineColor: '#ECECEC',
            minorGridLineColor: '#FAFAFA',
            minorTickInterval: 'auto'
        },
        rangeSelector: {
            buttons: [{
                type: 'hour',
                count: 1,
                text: '1h'
            }, {
                type: 'day',
                count: 1,
                text: '1d'
            }, {
                type: 'day',
                count: 2,
                text: '2d'
            }, {
                type: 'day',
                count: 3,
                text: '3d'
            }, {
                type: 'all',
                count: 1,
                text: 'All'
            }],
            selected: 1,
            inputEnabled: false
        },
        credits: {
            enabled: false
        },
        series: [{
            name: 'Current',
            type: 'line',
            data: getColumns(data, [0, 5]),
            tooltip: {
                valueDecimals: 2
            },
            lineWidth: 4,
            threshold: null,
            zIndex: 3,
            color: 'limeGreen'
        }, {
            name: 'Target',
            type: 'line',
            data: getColumns(data, [0, 4]),
            threshold: null,
            lineWidth: 4,
            zIndex: 1,
            color: '#DDDDDD',
            dashStyle: 'ShortDot'
        }, {
            name: 'Ambient',
            type: 'line',
            lineWidth: 4,
            data: getColumns(data, [0, 6]),
            threshold: null,
            zIndex: 2,
            color: 'lightBlue'


        }]
    });
}

//creates the charts that are updated with current data
function setupLiveCharts() {

    var gaugeOptions = {

        chart: {
            type: 'solidgauge',
            height: 150
        },
        title: null,
        pane: {
            center: ['50%', '65%'],
            size: '130%',
            startAngle: -90,
            endAngle: 90,
            background: {
                backgroundColor: (Highcharts.theme && Highcharts.theme.background2) || '#EEE',
                innerRadius: '60%',
                outerRadius: '100%',
                shape: 'arc'
            }
        },
        tooltip: {
            enabled: false
        },
        yAxis: {
            stops: [
                [0.1, '#55BF3B'], // green
                [0.5, '#DDDF0D'], // yellow
                [0.9, '#DF5353'] // red
            ],
            lineWidth: 0,
            minorTickInterval: null,
            tickPixelInterval: 200,
            tickWidth: 0,
            title: {
                y: 0
            },
            labels: {
                y: 15
            }
        },
        plotOptions: {
            solidgauge: {
                dataLabels: {
                    y: 0,
                    borderWidth: 0,
                    useHTML: true
                }
            }
        }
    };

    // The Current Temp Gauge - current.csv
    $('#container-current').highcharts(Highcharts.merge(gaugeOptions, {
        yAxis: {
            min: 50,
            max: 100,
            stops: [
                [0.1, '#55BF3B'], // green
                [0.5, '#DDDF0D'], // yellow
                [0.9, '#DF5353'] // red
            ]
        },
        credits: {
            enabled: false
        },
        series: [{
            name: 'Current Temp',
            data: [50],
            dataLabels: {
                format: '<div style="text-align:center"><span style="font-size:25px;color:' +
                    ((Highcharts.theme && Highcharts.theme.contrastTextColor) || 'black') + '">{y}</span></div>' // +
                    //'<span style="font-size:12px;color:silver">f</span></div>'
            },
            tooltip: {
                valueSuffix: ' f'
            }
        }]
    }));

    // The Ambient Temp Gauge - current.csv
    $('#container-ambient').highcharts(Highcharts.merge(gaugeOptions, {
        yAxis: {
            min: 50,
            max: 100,
            stops: [
                [0.1, '#55BF3B'], // green
                [0.5, '#DDDF0D'], // yellow
                [0.9, '#DF5353'] // red
            ]
        },
        credits: {
            enabled: false
        },
        series: [{
            name: 'Ambient',
            data: [1],
            dataLabels: {
                format: '<div style="text-align:center"><span style="font-size:25px;color:' +
                    ((Highcharts.theme && Highcharts.theme.contrastTextColor) || 'black') + '">{y:.1f}</span></div>'
            },
            tooltip: {
                valueSuffix: ' f'
            }
        }]
    }));

    // The Pump Status Gauge - current.csv
    $('#container-pump').highcharts(Highcharts.merge(gaugeOptions, {
        yAxis: {
            min: 0,
            max: 1,
            stops: [
                [0.9, '#55BF3B'] // green
            ],
            labels: {
                enabled: false
            }
        },
        credits: {
            enabled: false
        },
        series: [{
            name: 'Pump',
            data: [1],
            dataLabels: {
                formatter: function() {
                    if (this.y > 0) {
                        return '<div style="text-align:center"><span style="font-size:25px;">On</span></div>'
                    } else {
                        return '<div style="text-align:center"><span style="font-size:25px;">Off</span></div>'
                    }
                }
            }
        }]
    }));

    $('#container-pelt').highcharts(Highcharts.merge(gaugeOptions, {
        yAxis: {
            min: 0,
            max: 2,
            stops: [
                [0.5, '#0C6BBF'], // blue
                [0.9, '##FF0100'], // red
            ],
            labels: {
                enabled: false
            }
        },
        credits: {
            enabled: false
        },
        series: [{
            name: 'Pelt',
            data: [1],
            dataLabels: {
                formatter: function() {
                    if (this.y > 0) {
                        if (this.y == 1) {
                            return '<div style="text-align:center"><span style="font-size:25px;">Cool</span></div>'
                        } else {
                            return '<div style="text-align:center"><span style="font-size:25px;">Heat</span></div>'
                        }
                    } else {
                        return '<div style="text-align:center"><span style="font-size:25px;">Off</span></div>'
                    }
                }

            }
        }]
    }));

    //Run this every 30 seconds
    updateLiveCharts();
    setInterval(updateLiveCharts, 10000);

}

//updates live (current.csv) charts
function updateLiveCharts() {
    path = "data/current.csv";
    $.ajax({
        method: "GET",
        url: path,
        dataType: "text",
        success: function(data) {
            debug("Loaded: " + path);
            csvArray = parseCsvSimple(data, 1);

            if (csvArray.length == 1) {
                var point,
                    newVal,
                    inc;
                
                var testCurrentTemp;

                // push into current temp
                //if (currentChartTemp == "null"){
                    //alert("test");
                testCurrentTemp = $('#container-current').highcharts();
            //}
                //alert(testCurrentTemp);
                //if (testCurrentTemp) {
                    
                    point = testCurrentTemp.series[0].points[0];
                    point.y = csvArray[0][5];
                    point.update(newVal);
                //}

                // push into ambient temp
                chart = $('#container-ambient').highcharts();
                if (chart) {
                    point = chart.series[0].points[0];
                    point.y = csvArray[0][6];
                    point.update(newVal);
                }

                // push into pump temp
                chart = $('#container-pump').highcharts();
                if (chart) {
                    point = chart.series[0].points[0];
                    point.y = csvArray[0][7];
                    point.update(newVal);
                }

                // push into pelt status
                chart = $('#container-pelt').highcharts();
                if (chart) {
                    point = chart.series[0].points[0];
                    point.y = csvArray[0][8];
                    point.update(newVal);
                }

                if (lastUpdate != csvArray[0]) {
                    //this data is new
                    lastUpdate = csvArray[0];

                    //push points into history?
                }

                //more charts here
            }

        },
        fail: function() {
            debug("Error loading: " + path);
        }
    });
}

function setMailboxLink() {

    // Declare variables
    var mailboxURL = "";
    var mailboxArray = []; //array to hold everything
    var mailboxURLPrefix = "arduino.local/mailbox?"
    var mailboxInputID = "batchid=" + $('input[id="inputID"]').val()
    var mailboxInputName = "batchname=" + $('input[id="inputName"]').val()
    var mailboxInputSize = "batchsize=" + $('input[id="inputSize"]').val()
    var mailboxInputTarget = "targettemp=" + $('input[id="inputTarget"]').val()
    var mailboxInputTempRange = "tempdiff=" + $('input[id="inputTempRange"]').val()

    //place into an array
    if ($('input[id="inputID"]').val() !== "") {
        mailboxArray.push(mailboxInputID)
    }
    if ($('input[id="inputName"]').val() !== "") {
        mailboxArray.push(mailboxInputName);
    }

    if ($('input[id="inputSize"]').val() !== "") {
        mailboxArray.push(mailboxInputSize)
    }
    if ($('input[id="inputTarget"]').val() !== "") {
        mailboxArray.push(mailboxInputTarget)
    }
    if ($('input[id="inputTempRange"]').val() !== "") {
        mailboxArray.push(mailboxInputTempRange)
    }
    //Build URL
    mailboxURL = mailboxURLPrefix;
    if (mailboxArray.length > 1) {
        for (i = 0; i <= (mailboxArray.length - 1); i++) {
            if (i !== (mailboxArray.length - 1)) {
                mailboxURL += mailboxArray[i] + "&";
            } else {
                mailboxURL += mailboxArray[i]
            }
        }

    } else {
        mailboxURL += mailboxArray[0];
    }
    //replace link with mailboxURL

    $("#mailboxLink").attr("href", mailboxURL);

}

var enableDebugMessages = true; 	//Enable the debug log



var debugInt = 0;
//function debug(Message) {
//    if (enableDebugMessages) {
//
//        if (debugInt == 0) {
//            jQuery("html").append("<div id='DebugArea'><div class='DebugHeader DebugAreaButton'>HPN Debug</div><ol></ol></div>");
//            jQuery("#PageFooter").prepend("<span class='DebugAreaButton DebugHide'>_ </div>");
//
//            jQuery(".DebugAreaButton").click(function () {
//                ToggleDebug();
//            });
//        }
//
//        debugInt++;
//        if (Message.toLowerCase().indexOf("error") > 0) {
//            jQuery("#DebugArea ol").append("<li class='DebugMessage DebugMessageError'>" + Message + "</li>");
//        } else {
//            jQuery("#DebugArea ol").append("<li class='DebugMessage'>" + Message + "</li>");
//        }
//    }
//}

function debug(message) {
    $("#debug").append("<div class='debugMessage'>" + message + "</div>");
}

function debugAppend(Message) {
    if (enableDebugMessages) {
        if (debugInt == 0) {
            debug(Message);
        } else {
            jQuery("#debug ol li").last().append(Message);
        }
    }

}

function debugVisable() {
    return jQuery("#debug").is(":visible");
}

function ToggleDebug() {
    jQuery("#debug").slideToggle();
	
	/*
    if(keepAlive == false){
		keepAlive = true;
		foreverSession();
	}
    */
}

//binds a CTRL+ALT+(somthing) keypress to a function
function bindDebug(key, callback, args) {
    var isCtrl = false;
    var isAlt = false;
    
    jQuery(document).keydown(function(e) {
        if(!args) args=[]; // IE barks when args is null

        if(e.ctrlKey) isCtrl = true;
        if(e.altKey) isAlt = true;
        
        if(e.keyCode == key.charCodeAt(0) && isCtrl && isAlt) {
            callback.apply(this, args);
            return false;
        }
    }).keyup(function(e) {
        if(e.ctrlKey) isCtrl = false;
    });
}

/*_________________          _-_
\==============_=_/ ____.---'---`---.____
            \_ \    \----._________.----/
              \ \   /  /    `-_-'
          __,--`.`-'..'-_
         /____  USS BEER ||
              `--.____,*/