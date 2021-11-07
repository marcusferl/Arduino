/* ********************************************************* *
 * eventboard.js                                             *
 * (c) 2020 Markus Weik, markus@weik.de                      *
 * ********************************************************* */

const fs = require('fs');
const path = require('path');

const istr = ['','es ist '];
const iidx = 0; // show intro text before time
const mstr = ['','fünf ', 'zehn ', 'viertel ','zwanzig '];
const pstr = ['','vor ','nach '];
const dstr = ['','halb '];
const hstr = ['null','eins','zwei','drei','vier','fünf','sechs','sieben','acht','neun','zehn','elf','zwölf']
const tstr = ['',' uhr'];

/* sub routines */
function readJsonConfig(filename) {
    rawdata = fs.readFileSync(filename);
    return JSON.parse(rawdata);    
}

function customEncodeURIComponent(message) {
    for ( i=1; i <32; i++ ) {
        message = message.replace('{'+i.toString(19).toLowerCase()+'}', String.fromCharCode(i) );
    }
    return encodeURIComponent(message);
}

function makeRequest(host, message) {
    nocache= Date.now();
    requestUrl = 'http://'+host+'/?message='+customEncodeURIComponent(message)+'&nocache='+nocache;
    http = require('http');
    req = http.request(requestUrl, res => {
        if (res.statusCode === 200) {
            console.log('OK');
        }
    });
    req.on('error', error => {
        console.log(err);
    });
    req.end();
}

function getAdent(year) {
    christmasEveDay = new Date( year, 11, 24 ).getDay();
    return new Date( year, 11, 24-21-christmasEveDay );
}

function getEasterSunday(year) {
    var f = Math.floor,
        // Golden Number - 1
        G = year % 19,
        C = f(year / 100),
        // related to Epact
        H = (C - f(C / 4) - f((8 * C + 13)/25) + 19 * G + 15) % 30,
        // number of days from 21 March to the Paschal full moon
        I = H - f(H/28) * (1 - f(29/(H + 1)) * f((21-G)/11)),
        // weekday for the Paschal full moon
        J = (year + f(year / 4) + I + 2 - C + f(C / 4)) % 7,
        // number of days from 21 March to the Sunday on or before the Paschal full moon
        L = I - J,
        month = 3 + f((L + 40)/44),
        day = L + 28 - 31 * f(month / 4);
        month--; 
    return new Date( year, month, day );
};

function timeInWords( hours, minutes, mode=0 ) {

    midx = 0;
    pidx = 0;
    didx = 0;
    hidx = 0;
    tidx = 1;

    if (hours > 12) { hours = hours-12; }
    
    hidx = hours;
    
    if (minutes >  2)  { midx = 1; pidx=2; tidx=0; }             //  5 nach
    if (minutes >  7)  { midx = 2; }                             // 10 nach
    if (minutes > 12)  { 
        midx = 3;                                                // viertal nach
        if ( (minutes <= 17) & (mode === 1) ) {
            hidx++;
            pidx = 0;
        }
    }
    if (minutes > 17)  { midx = 4; }                             // 20 nach
    if (minutes > 22)  { midx = 1; pidx = 1; didx = 1; hidx++; } //  5 vor  halb 
    if (minutes > 27)  { midx = 0; pidx = 0; }                   //         halb
    if (minutes > 32)  { midx = 1; pidx = 2; }                   //  5 nach halb
    if (minutes > 37)  { midx = 4; pidx = 1; didx = 0; }         // 20 vor
    if (minutes > 42)  {                                         // 15 vor
        midx = 3; 
        if ( (minutes <= 47) & (mode === 1) ) {
            midx = 5;
            hidx++;
            pidx = 0;
        }        
    }   
    if (minutes > 47)  { midx = 2; }                             // 10 vor
    if (minutes > 52)  { midx = 1; }                             //  5 vor
    if (minutes > 57)  { midx = 0; pidx = 0; tidx = 1; }         //

    if (hidx > 12) {
        hidx=hidx-12;
    }
    if (hidx === 0) {
        hidx = 12;
    }
    return istr[iidx] + mstr[midx] + pstr[pidx] + dstr[didx] + hstr[hidx] + tstr[tidx];
}

function eventsSort( objArray ) {
    objArray.sort( function( a, b) {
        if ( (a.thisYearsDate > b.thisYearsDate) ) {
            return 1
        } else if (a.thisYearsDate == b.thisYearsDate) {
            return 0
        } else {
            return -1
        };
    });
};

function prepareEvents(array, compareDate) {
    // calculate just once in the loop
    const thisEaster = getEasterSunday( compareDate.getFullYear() );
    nextEaster = getEasterSunday( compareDate.getFullYear()+1 );
    thisAdvent = getAdent( compareDate.getFullYear() );
    nextAdvent = getAdent( compareDate.getFullYear()+1 );
    array.forEach(event => {
        if (!event.type) {
            event.type="once";
        }
        switch (event.type) {
            case "dynamic":
                switch (event.base) {
                    case "easter": 
                        event.thisYearsDate= new Date( thisEaster.getTime());
                        event.thisYearsDate.setDate(event.thisYearsDate.getDate() + event.days );
                        if (event.thisYearsDate < compareDate) {
                            event.thisYearsDate = new Date( nextEaster.getTime() );
                            event.thisYearsDate.setDate( event.thisYearsDate.getDate()  + event.days );
                        }
                        event.InitialDate = event.thisYearsDate;
                        break;
                    case "advent":
                        event.thisYearsDate= new Date( thisAdvent.getTime());
                        event.thisYearsDate.setDate(event.thisYearsDate.getDate() + event.days );
                        if (event.thisYearsDate < compareDate) {
                            event.thisYearsDate = new Date( nextAdvent.getTime() );
                            event.thisYearsDate.setDate( event.thisYearsDate.getDate()  + event.days );
                        }
                        event.InitialDate = event.thisYearsDate;
                        break;
                    default:
                }
                break;
            case "fixed":
                event.thisYearsDate = new Date( compareDate.getFullYear(), event.date.month-1, event.date.day );
                if (event.thisYearsDate < compareDate) {
                    event.thisYearsDate = new Date( compareDate.getFullYear()+1, event.date.month-1, event.date.day );
                };
                event.InitialDate = new Date( event.date.year, event.date.month-1, event.date.day );
                break;
            case "once":
            default:
                event.InitialDate = new Date( event.date.year, event.date.month-1, event.date.day );
                event.thisYearsDate =event.InitialDate;
        }
    });
    eventsSort(array);
};

function getNextEvent (array, compareDate) {
    lid = 0;
    next = array[lid];
    while (next.thisYearsDate < compareDate) {
         lid++;
         next = array[lid];
    }
    return array[lid];
};

function getEventString(event, compareDate) {
    hours = Math.floor( (event.thisYearsDate-compareDate)/(1000*60*60));
    years = Math.floor( (event.thisYearsDate-event.InitialDate)/(1000*60*60*24*365.25));
    if (event.thisYearsDate > compareDate) {
        return event.message.replace('{{name}}', event.name).replace('{{hours}}', hours).replace('{{years}}', years);
    } else {
         return event.name.replace('{{hours}}', hours).replace('{{years}}', years);
    }
};

function main() {
    baseDirectory = path.dirname(process.argv[1]);
    eventBoard = readJsonConfig(path.join('', baseDirectory, 'config.json'));//
    cmdLineParams = process.argv.slice(2);
    if (cmdLineParams.length > 0) {
        cmdLineParams.forEach( parameter => {
            pair = parameter.split('=');
            if (pair.length == 2) {
                switch (pair[0].toLowerCase()) {
                    case 'message':
                        eventBoard.defaultMessage = pair[1];
                        break;
                    case 'host':
                        eventBoard.defaultHost = pair[1].toLowerCase();
                        break
                    default:
                        console.log( 'Parameter [' + parameter + '] is not supported.' );
                        break;
                }
            } else {
                console.log( 'Parameter [' + parameter + '] is not supported.' );
            }
        });
    }
    // eventBoard.defaultMessage = 'Halloween{1}';
    if (eventBoard.defaultMessage  === '') {
        eventsJson = readJsonConfig(path.join('', baseDirectory, 'events.json'));
        holidaysJson = readJsonConfig(path.join('', baseDirectory, 'holidays.json'));
        // merge events and holidays arrays
        eventsJson.events = eventsJson.events.concat(holidaysJson.holidays);
        // merge events to eventBoards objects
        eventBoard = Object.assign({}, eventBoard, eventsJson)  ;
        today = new Date();
        yesterday = new Date( today.getTime() );
        yesterday.setDate( yesterday.getDate() -1);
        prepareEvents(eventBoard.events, yesterday);
        next = getNextEvent (eventBoard.events, yesterday);
        eventBoard.defaultMessage=getEventString(next, today);
    } else {
        if (eventBoard.defaultMessage.toLowerCase()  === '{timeinwords}') {
            var now = new Date();
            
            eventBoard.defaultMessage = timeInWords( now.getHours(), now.getMinutes() );
        }
    }
    makeRequest(eventBoard.defaultHost, eventBoard.defaultMessage);
};

/* main :) */
main();

/* eof */
