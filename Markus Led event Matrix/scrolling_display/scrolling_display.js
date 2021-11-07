var cmdLineParams = process.argv.slice(2);
var message=encodeURIComponent('Hallo Markus!');
var nocache=encodeURIComponent('y');
var host='devboard.fritz.box'

function makeRequest() {

    requestUrl = 'http://'+host+'/?message='+message+'&nocache='+nocache;

    const http = require('http');
    const req = http.request(requestUrl, res => {
        if (res.statusCode === 200) {
            console.log('OK');
        }
    //   res.on('data', d => {
    //     process.stdout.write(d)
    //   })
    });

    req.on('error', error => {
    console.error(error)
    });
    req.end();
    
}

if (cmdLineParams.length < 1) {
    console.log( 'Parameter required: [host=devboard] message="A custom message"');
    process.exit(1);
}

if (cmdLineParams.length > 0) {
    cmdLineParams.forEach( parameter => {
        pair = parameter.split('=');
        if (pair.length == 2) {
            switch (pair[0].toLowerCase()) {
                case 'message':
                    message = encodeURIComponent(pair[1]);
                    break;
                case 'host':pair[1].toLowerCase();
                    break
                default:
                    console.log( 'parameter ' + pair + ' is not supported by this version.' );
                    break;
            }
        }
    });
}

makeRequest();
