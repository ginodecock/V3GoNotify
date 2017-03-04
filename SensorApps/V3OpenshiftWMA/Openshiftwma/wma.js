var express = require('express')
//var enforce = require('express-sslify')
var bodyParser = require('body-parser')
var path = require('path')
var ipaddress = process.env.OPENSHIFT_NODEJS_IP;
var port = process.env.OPENSHIFT_NODEJS_PORT || 8080;
console.log (port);
var production = true;
if (typeof ipaddress === "undefined") {
	production = false;
	ipaddress = "192.168.168.21"
};
console.log (ipaddress);
var mongoose = require('mongoose');
var passport = require('passport');
var flash    = require('connect-flash');

var morgan       = require('morgan');
var cookieParser = require('cookie-parser');
var bodyParser   = require('body-parser');
var session      = require('express-session');
var Sensorlog = require('./app/models/sensorlog');
var Sensor = require('./app/models/sensor');
var PushBullet = require('pushbullet');
var cronJob = require('cron').CronJob;
var moment = require('moment');

var app = express();
//app.use(enforce.HTTPS({ trustProtoHeader: true }));

app.use(bodyParser.json());
app.use(express.static(path.join(__dirname,'public')));
app.use(morgan('dev')); // log every request to the console
app.use(cookieParser()); // read cookies (needed for auth)
app.use(bodyParser()); // get information from html forms

//app.set('views', path.join(__dirname , '/view'));
app.set('view engine', 'ejs');
app.locals.pretty = true;

// required for passport
app.use(session({ secret: 'ilovescotchscotchyscotchscotch' })); // session secret
app.use(passport.initialize());
app.use(passport.session()); // persistent login sessions
app.use(flash()); // use connect-flash for flash messages stored in session
app.locals.moment = moment;
app.locals.dateFormat = function(date){
        return moment(date).format('x');
    }
app.locals.humanDateFormat = function(date){
        return moment(date).format('YYYY-MM-DD hh:mm');
    }
// default to a 'localhost' configuration:
var connection_string = '127.0.0.1:27017/wma';
// if OPENSHIFT env variables are present, use the available connection info:
if(process.env.OPENSHIFT_MONGODB_DB_PASSWORD){
  connection_string = process.env.OPENSHIFT_MONGODB_DB_USERNAME + ":" +
  process.env.OPENSHIFT_MONGODB_DB_PASSWORD + "@" +
  process.env.OPENSHIFT_MONGODB_DB_HOST + ':' +
  process.env.OPENSHIFT_MONGODB_DB_PORT + '/' +
  process.env.OPENSHIFT_APP_NAME;
}
console.log(connection_string);
var mongojs = require('mongojs');
var db = mongojs(connection_string, ['books']);
var books = db.collection('books');
// similar syntax as the Mongo command-line interface
// log each of the first ten docs in the collection
db.books.find({}).limit(10).forEach(function(err, doc) {
  if (err) throw err;
  if (doc) { console.dir(doc); }
});

mongoose.connect(connection_string);
require('./config/passport')(passport); // pass passport for configuration
//require('./app/models/sensorlog')(Sensorlog);

app.post('/wma',function(req,res) {
	var success = false
	console.log('request:');
	console.log(req.body);
	var rebootOffset = 0;
	Sensor.findOne({sensorId:req.body.chipId}, function(err, sensorres){
        console.log("++" + sensorres.rebootoffset + "++");
        rebootOffset = sensorres.rebootoffset;    	
    	var parsedBody = req.body
		if (parsedBody.chipId && parsedBody.status){
			var chipId = parsedBody.chipId;
			var value1 = parsedBody.v1 + rebootOffset;
			var value2 = parsedBody.v2/100;
			var value3 = parsedBody.v3;
			var status = parsedBody.status;
			var currentDate = Date();
			var logNow = new Sensorlog({
				sensorId: chipId,
				value1: value1,
				value2: value2,
				value3: value3,
				status: status,
				timestamp: currentDate
			});
			logNow.save(function(err) {
  				if (err) throw err;
  				console.log('Sensorlog saved successfully!');
			});
		}

		if (parsedBody.status.substring(0, 5) == "alarm"){
			console.log("alarm detected");
			if (sensorres.pbnotify && sensorres.pbid != ""){
            	console.log(sensorres.pbid);
            	var pusher = new PushBullet(sensorres.pbid);
            	pusher.note('', sensorres.name, parsedBody.status + '! Water usage = ' + parsedBody.v1 + ' liter @ Temperature = ' + parsedBody.v2/100 +'°C', function(error, response) {
					console.log(response); 
				});
            }
		}
		if ((parsedBody.status == "alarmBoot") || (parsedBody.status == "alarmTrigger") ){
			console.log("set reboot offset");
			Sensorlog.findOne({ $query: {sensorId:parsedBody.chipId, status:"log"}, $orderby:{timestamp:-1}},function(err, sensorlogres){
				var query = {sensorId:parsedBody.chipId};
        		var update = {rebootoffset: sensorlogres.value1};
        		var options = {new: true};
        		Sensor.findOneAndUpdate(query, update, options, function(err, res) {
            		if (err) {
            		console.log('got an error');
            		}
        		});
        	});
		}
	});
	//res.write('\n');
	var d1 = new Date();
	var startDate = moment(d1);
	d1.setMinutes(0);
	d1.setSeconds(0);
	d1.setHours(d1.getHours() + 1);
	var endDate = moment(d1);
	var secondsDiff = endDate.diff(startDate, 'seconds');
	console.log('seconds to : '+ secondsDiff);
	res.removeHeader("X-Powered-By");
	res.removeHeader("Set-Cookie");
	res.setHeader('Nextlog', Math.abs(secondsDiff).toString() + 's');
	//res.json({nextlog : secondsDiff});
	res.end('{"nextlog":' + Math.abs(secondsDiff).toString()+'}');
})


app.get('/a',function(req,res) {
	//sleep(2000);
	//
	res.writeHead(200, {'Content-Type': 'text/html'});
    res.end('get received');
})

var checkDeadSensors = new cronJob('0 0 * * * *', function(){
	console.log(Date())
	Sensor.find({}, function(err, sensors){
        if (err) throw err;
//        console.log(sensors);
        sensors.forEach(function(sensor) {
      		if (sensor.pbnotify && sensor.pbid != ""){
      			var d = new Date();
				var d2 = new Date();
				d2.setHours(d.getHours() - 2);	
      			Sensorlog.count({sensorId:sensor.sensorId,timestamp: {$gt:d2}}, function(err, number){
      				if (number == 0){
      					console.log('deadalert '+ sensor.name + ' pbid: ' + sensor.pbid);	
      					var pusher = new PushBullet(sensor.pbid);
            			pusher.note('', sensor.name, "Is not communicating", function(error, response) {
							console.log(response); 
						});
      				}
      			});
      		}
    	});
    });
});
checkDeadSensors.start();


// routes ======================================================================
require('./app/routes.js')(app, passport); // load our routes and pass in our app and fully configured passport

app.listen(port,ipaddress)



console.log('The magic happens on port ' + port);
