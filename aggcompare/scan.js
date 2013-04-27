'use strict';
//To compare gt would require regenerating values from expected bubbles.

//Using mongo to jsonfiy the csv:
//mongoimport -d mydb -c aggdata --type csv --file aggdata.csv --headerline
//mongoexport --db mydb --collection aggdata --jsonArray --out aggdata.json

var fs = require('fs');
var path = require('path');
//Database:
var mongo = require('mongoskin');
var _ = require('underscore');
var db;


var walk = function(dir, done) {
  var results = [];
  fs.readdir(dir, function(err, list) {
    if (err) return done(err);
    var pending = list.length;
    if (!pending) return done(null, results);
    list.forEach(function(file) {
      var filepath = dir + '/' + file;
      if (file === 'output.json') {
        fs.readFile(filepath, 'utf8', function (err, data) {
          if (err) return done(err);
          var pageName = path.basename(dir);
          var name = pageName.replace("(page2)", '');
          var nameComponents = name.split('_');
          var time = nameComponents[2].replace(/-/g, ':');
          var timestamp = new Date(nameComponents[1] + ' ' + time + ' UTC+0200');

          var formObject = Object.create({
            __name__ : name,
            __page__ : pageName.match("(page2)") ? 2 : 1,
            __dataset__ : path.basename(path.dirname(dir)),
            __timestamp__ : timestamp,
            __path__ : filepath
          });

          JSON.parse(data).fields.forEach(function(field){
            if(!("value" in field)) return;
            var flabel = field.name;
            if(flabel == "provincia") return;
            if(flabel == "distrito") return;
            if(flabel == "communidade") return;
            if(flabel == "APE_name") return;
            if(flabel == "mes") return;
            if(flabel == "ano") return;

            formObject[field.name] = field.value;
          });
          results.push(formObject);
          if (!--pending) done(null, results);
        });
        return;
      }

      fs.stat(filepath, function(err, stat) {
        if (stat && stat.isDirectory()) {
          walk(filepath, function(err, res) {
            if (err) return done(err);
            results = results.concat(res);
            
            if (!--pending) done(null, results);
          });
        } else {
          if (!--pending) done(null, results);
        }
      });
    });
  });
};
walk('../tests/MozExperiment_out', function(err, pages) {
  if (err) console.log(err);

  //Combine the pages into a single json objects.
  var scanForms = _.values(_(pages).groupBy('__name__')).map(function(pages){
    if(pages.length == 2) {
      pages[1].__bothPages__ = true;
      return _.extend(pages[0], pages[1]);
    }
    console.log(pages[0].__name__);
    return _.extend(pages[0]);
  });

  //Skip single page forms...
  scanForms = _.where(scanForms, {__bothPages__: true});

  fs.readFile('aggdata.json', 'utf8', function (err, data) {
    if (err) return console.log(err);
    var aggData = JSON.parse(data);

    var totalCheckboxFields = 0;
    var totalCorrectCheckboxFields = 0;
    var totalNumericFields = 0;
    var totalCorrectNumericFields = 0;

    scanForms.slice(0).forEach(function(scanForm){
      var minTimeDiff = Infinity;
      var timeMatch;
      var mostMatchingFields = 0;
      var fieldMatch;
      aggData.forEach(function(aggForm){

        //distrito?
        var matchingFields = _.reduce(_.pairs(scanForm), function(sum, field){
          if(_.isUndefined(field[1])) {
            return sum;
          } else if(_.isNumber(field[1])) {
            return field[1] === aggForm[field[0]] ? sum + 1 : sum;
          } else {
            return sum;
          }
        }, 0);

        if(matchingFields > mostMatchingFields){
          mostMatchingFields = matchingFields;
          fieldMatch = aggForm;
        }

        var differenceInCounts = _.reduce(_.pairs(scanForm), function(sum, field){
          if(_.isUndefined(field[1])) {
            return sum;
          } else if(_.isNumber(field[1])) {
            return Math.abs(field[1] - aggForm[field[0]]) + sum;
          } else {
            return sum;
          }
        }, 0);

        var timeDiff = Math.abs(new Date(scanForm.__timestamp__) - new Date(aggForm.xformendtime));
        if(timeDiff < minTimeDiff){
          minTimeDiff = timeDiff;
          timeMatch = aggForm;
        }

      });

      //Check if the matches changed.
      //Use this after running with gt data so we don't mismatch on bad data.
      var prevMatch = _.where(aggData, {__previousMatch__: scanForm.__name__})[0];
      if(prevMatch !== fieldMatch) {
        if(true){
          console.log("Previous match is bad.");
          fieldMatch = prevMatch;
        } else {
          //Remove the previous previous match
          _.where(aggData, {__previousMatch__: scanForm.__name__}).forEach(function(match){
            match.__previousMatch__ = null;
          });
          //Annotate agg data with previous match
          fieldMatch.__previousMatch__ = scanForm.__name__;
        }
      }

      //Time Difference:
      var xformFinishTimeDelta = ( new Date(fieldMatch.xformendtime) - scanForm.__timestamp__ ) / (60 * 60 * 1000);
      //Error:
      var numericFieldDiffs = {};
      var checkboxErrors = 0;
      var checkboxFields = 0;

      _.pairs(scanForm).forEach(function(field){
        //Ignore the fields I added.
        if(field[0].slice(0,2) === "__") return;
        var otherValue = fieldMatch[field[0]];
        if(_.isUndefined(otherValue)){ 
          console.log(field[0]);
        }
        if(_.isUndefined(field[1])) {
          //noop
        } else if(_.isNumber(field[1])) {
          //The aggregate data has some random nulls peppered thoughout it.
          //I'm treating them as 0s
          if(otherValue === "null") {
            otherValue = 0;
          }
          var diff = field[1] - otherValue;
          if(_.isNaN(diff)) console.log(field[1], otherValue);
          numericFieldDiffs[diff] = (diff in numericFieldDiffs) ? numericFieldDiffs[diff] + 1 : 1;
        } else if(_.isString(field[1])) {
          if(otherValue != field[1]) checkboxErrors++;
          checkboxFields++;
        }
      });

      //Write to path:
      /*
      fs.writeFile(path.join(path.dirname(scanForm.__path__), "aggData.json"),
        JSON.stringify(_.extend(fieldMatch, {
          "xformFinishTimeDelta": xformFinishTimeDelta,
          "checkboxErrors": checkboxErrors,
          "numericFieldDiffs": numericFieldDiffs
        }), 0, 2),
        function(err) {
          if(err) {
              console.log(err);
          }
      });
      */
/*
      if(xformFinishTimeDelta > 550 || xformFinishTimeDelta < -1){
        //There is one form with a slightly negative time
        //It's text fields check out though so I'm not sure what the deal is...
        //Everything seems to spot
        console.log(fieldMatch);
        console.log(scanForm.__name__);
      }

      console.log({
        "Difference in hours between scan time and xform finish time":xformFinishTimeDelta,
        "Number of errors in checkbox fields":checkboxErrors,
        "Accuracy of numeric fields":numericFieldDiffs,
        //fieldMatch: fieldMatch,
        //scanForm: scanForm.__name__
      });
*/
      totalCheckboxFields += checkboxFields;
      totalCorrectCheckboxFields += checkboxFields - checkboxErrors;

      totalCorrectNumericFields += numericFieldDiffs[0];
      totalNumericFields += _.values(numericFieldDiffs).reduce(function(a, b) {
          return a + b;
      });



    });

    console.log({
      totalCorrectCheckboxFields: totalCorrectCheckboxFields,
      totalIncorrectCheckboxFields: totalCheckboxFields - totalCorrectCheckboxFields,
      totalCorrectNumericFields: totalCorrectNumericFields,
      totalIncorrectNumericFields: totalNumericFields - totalCorrectNumericFields,
      "% checkbox fields correct": totalCorrectCheckboxFields / totalCheckboxFields,
      "% bubble tally fields correct" : totalCorrectNumericFields / totalNumericFields
    });

    fs.writeFile('aggdata.json', JSON.stringify(aggData), function(err) {
        if(err) {
            console.log(err);
        }
    });


  });
});
