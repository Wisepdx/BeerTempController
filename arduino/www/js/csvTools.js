/*___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|
___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|__
_|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|
___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|__
_|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|
___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|__
_|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|
___|___|___|___|___|__|____|___|___|___|___|___|___|___|___|___|___|__
_|___|___|___|___|___|__/-------------------\_|__|___|___|___|___|___|
___|___|___|___|___|___|  THE GREAT BEERWALL |_|___|___|___|___|___|__
_|___|___|___|___|___|__\-------------------/|___|___|___|___|___|___|
___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|__
_|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|
___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|__
_|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|
___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|__
_|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|
___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___|___*/

var csvString = "";

//-------------- CSV STUFF

//Takes a CSV and converts it to an JS array
function parseCsvSimple(csvString, ignoreHeaderRows){
    if(csvString === "undefined"){
      debug("Error parsing CSV string, the string was undefined.");
      return null;
    }
  
    var lines = csvString.split('\n');
    var output = new Array();
    
    for(var i = ignoreHeaderRows; i < lines.length; i++){
        //for each line
        lines[i] = lines[i].split('\r').join('')
        var items = lines[i].split(',');
        var outputLine = new Array();
    
        for(var i2 = 0; i2 < items.length; i2++){
            //for each item in the line
            outputLine[i2] = parseSomthing(items[i2]);
        }
        
        output[i-ignoreHeaderRows] = outputLine;
    }
    
    return output;
}

//takes an unknown string and returns an object
function parseSomthing(input){
    var f = parseFloat(input);
    var d = new Date(input);
    
    if((input.toString().indexOf(":") > 0) && (!isNaN(d))) {
        return d.getTime(); 
        //note: this returns a EPOCH formatted number, which is the only kind of date highcharts understands
    } else if(!isNaN(f)){ 
        return f; //A number or float
    } else { 
        return input; //somthing else, maybe a string or null
    }
}

//Selects a column from a multidimentional array
function getColumn(jsArray, columnIndex){
    var output = new Array();
    
    for(var i = 0; i < jsArray.length; i++){
        if(jsArray[i].length >= columnIndex){
            output[i] = jsArray[i][columnIndex]
        }
    }
    
    return output;
}

//Selects any dimentions (columns) from a multidimentional array
function getColumns(jsArray, columnIndexes){
    var output = new Array();
    
    for(var i = 0; i < jsArray.length; i++){
        var pointArray = new Array();
        
        for(var i2 = 0; i2 < columnIndexes.length; i2++) {    
            if(i2 >= 0 && jsArray[i2].length >= i2) {
                pointArray[i2] = jsArray[i][columnIndexes[i2]];
            }
        }
        
        output[i] = pointArray;
    }
    
    return output;
}

 /*_________________          _-_
 \==============_=_/ ____.---'---`---.____
             \_ \    \----._________.----/
               \ \   /  /    `-_-'
           __,--`.`-'..'-_
          /____  USS BEER ||
               `--.____,*/  
