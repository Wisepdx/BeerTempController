
function debug(message){
  $("#debug").append("<div class='debugMessage'>" + message + "</div>")
}

var csvArray;
var csvString;
var currentCsvArray;
var lastUpdate;

function loadFile(path){
    $.ajax({
    method: "GET",
    url: path,
    dataType: "text",
    success: function(data) {
      debug("Loaded: " + path);
      csvString = data;
      csvArray = parseCsvSimple(data, 1);
      createHistoryChart(csvArray);
    }, fail: function() {
      debug("Error loading data");
    }
  });
}

$(document).ready(function () {
  
  //load and parse the data
    loadFile("data/1.csv");
    
  $("#loadLink").click(function(){
    loadFile($("#loadBox").val());
  });
  
  setupLiveCharts();
  updateLiveCharts();
});

// create the actual chart after the data is loaded
function createHistoryChart(data){
  
        $('#container').highcharts('StockChart', {
            //title: {
            //    text: 'Batch #' + getColumn(data,1)[0] + " History",
            //}, 
            subtitle: {
                text: ''
            }, xAxis: {
                gapGridLineWidth: 0
            }, yAxis: {
                gridLineColor: '#ECECEC',
                minorGridLineColor: '#FAFAFA',
                minorTickInterval: 'auto'
            }, rangeSelector : {
                buttons : [{
                    type : 'hour',
                    count : 1,
                    text : '1h'
                }, {
                    type : 'day',
                    count : 1,
                    text : '1d'
                }, {
                    type : 'day',
                    count : 2,
                    text : '2d'
                }, {
                    type : 'day',
                    count : 3,
                    text : '3d'
                }, {
                    type : 'all',
                    count : 1,
                    text : 'All'
                }],
                selected : 1,
                inputEnabled : false
            }, credits: {
               enabled: false
            }, series : [{
                name : 'Current',
                type: 'line',
                data : getColumns(data,[0,5]),
                tooltip: {
                    valueDecimals: 2
                },
                lineWidth: 4,
                threshold: null,
                zIndex: 3,
                color: 'limeGreen'
            },{
                name : 'Target',
                type: 'line',
                data : getColumns(data,[0,4]),
                threshold: null,
                lineWidth: 4,
                zIndex: 1,
                color: '#DDDDDD',
                dashStyle: 'ShortDot'
            },{
                name : 'Ambient',
                type: 'line',
                lineWidth: 4,
                data : getColumns(data,[0,6]),
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
        }, title: null,
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
        }, tooltip: {
            enabled: false
        }, yAxis: {
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
            }, labels: {
                y: 15
            }
        }, plotOptions: {
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
        }, credits: {
            enabled: false
        }, series: [{
            name: 'Current Temp',
            data: [50],
            dataLabels: {
                format: '<div style="text-align:center"><span style="font-size:25px;color:' +
                    ((Highcharts.theme && Highcharts.theme.contrastTextColor) || 'black') + '">{y}</span></div>'// +
                       //'<span style="font-size:12px;color:silver">f</span></div>'
            }, tooltip: {
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
        }, credits: {
            enabled: false
        }, series: [{
            name: 'Ambient',
            data: [1],
            dataLabels: {
                format: '<div style="text-align:center"><span style="font-size:25px;color:' +
                    ((Highcharts.theme && Highcharts.theme.contrastTextColor) || 'black') + '">{y:.1f}</span></div>'
            }, tooltip: {
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
            labels:{
                enabled: false
            }
        }, credits: {
            enabled: false
        }, series: [{
            name: 'Pump',
            data: [1],
            dataLabels: {
                formatter: function(){
                    if(this.y > 0){
                        return '<div style="text-align:center"><span style="font-size:25px;">On</span></div>'
                    }else{
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
                [0.5, '#FF0100'], // red
                [0.9, '#0C6BBF'], // blue
            ],
            labels:{
                enabled: false
            }
        }, credits: {
            enabled: false
        }, series: [{
            name: 'Pelt',
            data: [1],
            dataLabels: {
                formatter: function(){
                    if(this.y > 0){
                        if(this.y == 1){
                         return '<div style="text-align:center"><span style="font-size:25px;">Heat</span></div>'
                        } else{
                         return '<div style="text-align:center"><span style="font-size:25px;">Cool</span></div>'
                        }
                    }else{
                     return '<div style="text-align:center"><span style="font-size:25px;">Off</span></div>'
                    }
                }
                
            }
        }]
    }));

    //Run this every 30 seconds
    setInterval(updateLiveCharts, 30000);

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
      currentCsvArray = parseCsvSimple(data, 1);
      
      if(currentCsvArray.length == 1){
        var point,
            newVal,
            inc;
        
        // push into current temp
        var chart = $('#container-current').highcharts();
        if (chart) {
            point = chart.series[0].points[0];
            point.y = currentCsvArray[0][5];
            point.update(newVal);
        }
  
        // push into ambient temp
        chart = $('#container-ambient').highcharts();
        if (chart) {
            point = chart.series[0].points[0];
            point.y = currentCsvArray[0][6];
            point.update(newVal);
        }
        
        // push into pump temp
        chart = $('#container-pump').highcharts();
        if (chart) {
            point = chart.series[0].points[0];
            point.y = currentCsvArray[0][7];
            point.update(newVal);
        }

        // push into pelt status
        chart = $('#container-pelt').highcharts();
        if (chart) {
            point = chart.series[0].points[0];
            point.y = currentCsvArray[0][8];
            point.update(newVal);
        }

        if(lastUpdate != currentCsvArray[0]){
          //this data is new
          lastUpdate = currentCsvArray[0];
          
          //push points into history?
        }
  
        //more charts here
      }
  
    }, fail: function() {
      debug("Error loading: " + path);
    }
  });

}

 /*_________________          _-_
 \==============_=_/ ____.---'---`---.____
             \_ \    \----._________.----/
               \ \   /  /    `-_-'
           __,--`.`-'..'-_
          /____  USS BEER ||
               `--.____,*/  
