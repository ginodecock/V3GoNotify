module.exports = function(app, passport) {

// normal routes ===============================================================
    var ejs = require('ejs')
        , moment = require('moment');

    /*app.locals.fromNow = function(date){
        return moment(date).fromNow();
    }*/

    // show the home page (will also have our login links)
    app.get('/', function(req, res) {
        res.render('index.ejs');
    });

    // PROFILE SECTION =========================
    var Sensor = require('../app/models/sensor');
    var Users = require('../app/models/user');
    app.get('/profile', isLoggedIn, function(req, res) {

        Sensor.find({user_id:req.user._id }, function(err, sensors){
            if (err) throw err;
            console.log(sensors);
            res.render('profile.ejs', {
            user : req.user,
            sensors: sensors
            });
        });
    });

    // LOGOUT ==============================
    app.get('/logout', function(req, res) {
        req.logout();
        res.redirect('/');
    });
    
    var Sensorlog = require('../app/models/sensorlog');
    app.get('/getWma',isLoggedIn, function(req,res) {
        Sensorlog.find({}, function(err, data){
        if (err) throw err;
        console.log(data);
        res.render('data',{data:data});
        });
    });
    app.post('/logmysensor',isLoggedIn, function(req,res) {
        Sensorlog.find({sensorId:req.body.sensorId}, function(err, data){
        if (err) throw err;
        console.log(req.body.sensorId);
        console.log(data);
        res.render('data',{data:data});
        });
    });
     app.post('/controlmysensor',isLoggedIn, function(req,res) {
        console.log(req.body);
        var sensor;
        Sensor.findOne({sensorId:req.body.sensorId}, function(err, ressensor){
            if (err) throw err;
            console.log(req.body.sensor);
            console.log(ressensor);
            sensor = ressensor;
            });


        if (req.body.request == "Log"){
            Sensorlog.find({ $query: {sensorId:req.body.sensorId, status:"log"}, $orderby:{timestamp:-1}},{},{limit:100}, function(err, sensorlogs){
                if (err) throw err;
                console.log(sensor);

                res.render('wmasensorlog.ejs',{
                    sensor: sensor,
                    sensorlogs: sensorlogs,
                });
            });
        }
        if (req.body.request == "Alarm"){
            Sensorlog.find({ $query: {sensorId:req.body.sensorId, status: /^alarm/}, $orderby:{timestamp:-1}},{},{limit:100}, function(err, sensorlogs){
                if (err) throw err;
                console.log(sensor);
                res.render('wmasensoralarm.ejs',{
                    sensor: sensor,
                    sensorlogs: sensorlogs,
                });
            });
        }
        if (req.body.request == "Graph"){
            Sensorlog.find({ $query: {sensorId:req.body.sensorId, status:"log"}, $orderby:{timestamp:-1}},{},{limit:100}, function(err, sensorlogs){
                if (err) throw err;
                console.log(sensor);

                res.render('wmasensorgraph.ejs',{
                    sensor: sensor,
                    sensorlogs: sensorlogs.reverse()
                });
            });
        }
        if (req.body.request == "Battery"){
            Sensorlog.find({sensorId:req.body.sensorId, status:"log"}, function(err, sensorlogs){
                if (err) throw err;
                console.log(sensor);

                res.render('wmasensorbattery.ejs',{
                    sensor: sensor,
                    sensorlogs: sensorlogs
                });
            });
        }
        if (req.body.request == "Config"){
            Sensor.find({sensorId:req.body.sensorId}, function(err, sensors){
            if (err) throw err;
            console.log(req.body.sensor);
            console.log(sensors);
            res.render('config',{sensors: sensors});
            });
        }

        
    });


    app.post('/deletemysensor',isLoggedIn, function(req,res) {
        console.log(req.body);
        console.log(req.body._id);
        Sensor.findOneAndRemove({_id : req.body._id },
            function (err, user){
                if (!err) {
                console.log(" removed");
                };
            }
        );
        res.redirect('/profile'); // redirect to the secure profile section
       
    });
    app.post('/deletemyuser',isLoggedIn, function(req,res) {
        console.log(req.body);
        console.log(req.body.userid);
        Users.findOneAndRemove({_id : req.body.userid },
            function (err, user){
                if (!err) {
                console.log(" removed");
                };
            }
        );
        res.redirect('/profile'); // redirect to the secure profile section
       
    });
    app.post('/updatemysensor',isLoggedIn, function(req,res) {
        console.log(req.body);
        console.log(req.body._id);

        var query = {"_id": req.body._id};
        var update = {pbid: req.body.pbid,pbnotify: req.body.pbnotify,name: req.body.name,type: req.body.type,meteroffset: req.body.meteroffset, rebootoffset: req.body.rebootoffset};
        var options = {new: true};
        Sensor.findOneAndUpdate(query, update, options, function(err, res) {
            if (err) {
            console.log('got an error');
            }
        });
        res.redirect('/profile');// redirect to the secure profile section
       
    });

    app.get('/getsensors',isLoggedIn,function(req,res) {
        Sensor.find({}, function(err, data){
        if (err) throw err;
        console.log(data);
        res.render('data',{data:data});
        });
    });
    app.get('/getmysensors',isLoggedIn,function(req,res) {
        Sensor.find({user_id:req.user._id }, function(err, data){
        if (err) throw err;
        console.log(data);
        res.render('data',{data:data});
        });
    });
    app.get('/getmyusers',isLoggedIn,function(req,res) {
        Users.find({}, function(err, data){
        if (err) throw err;
        console.log(data);
        res.render('data',{data:data});
        });
    });
    app.post('/addsensor',isLoggedIn,function(req,res){
        var name=req.body.name;
        var sensorId=req.body.sensorId;
        var type=req.body.type;
        console.log("sensorId = " + sensorId + "name = " + name + " type = " + type);
        var currentDate = new Date();
        var sensorAdd = new Sensor({
                sensorId: sensorId,
                name: name,
                type: type,
                timestamp: currentDate,
                user_id: req.user._id
            });
            sensorAdd.save(function(err) {
                if (err) throw err;
                console.log('Sensor added successfully!');
                console.log(sensorAdd);
            });
        res.redirect('/profile'); // redirect to the secure profile section
        
    });
    app.post('/newsensor',isLoggedIn,function(req,res){
        res.render('newsensor.ejs');
    });


// =============================================================================
// AUTHENTICATE (FIRST LOGIN) ==================================================
// =============================================================================

    // locally --------------------------------
        // LOGIN ===============================
        // show the login form
        app.get('/login', function(req, res) {
            res.render('login.ejs', { message: req.flash('loginMessage') });
        });

        // process the login form
        app.post('/login', passport.authenticate('local-login', {
            successRedirect : '/profile', // redirect to the secure profile section
            failureRedirect : '/login', // redirect back to the signup page if there is an error
            failureFlash : true // allow flash messages
        }));

        // SIGNUP =================================
        // show the signup form
        app.get('/signup', function(req, res) {
            res.render('signup.ejs', { message: req.flash('signupMessage') });
        });

        // process the signup form
        app.post('/signup', passport.authenticate('local-signup', {
            successRedirect : '/profile', // redirect to the secure profile section
            failureRedirect : '/signup', // redirect back to the signup page if there is an error
            failureFlash : true // allow flash messages
        }));
        app.get('/changesignup', function(req, res) {
            res.render('changesignup.ejs', { message: req.flash('changesignupMessage'), user : req.user});
        });

        // process the signup form
        app.post('/changesignup', passport.authenticate('local-changesignup', {
            successRedirect : '/profile', // redirect to the secure profile section
            failureRedirect : '/changesignup', // redirect back to the signup page if there is an error
            failureFlash : true // allow flash messages
        }));


    
// =============================================================================
// AUTHORIZE (ALREADY LOGGED IN / CONNECTING OTHER SOCIAL ACCOUNT) =============
// =============================================================================

    // locally --------------------------------
        app.get('/connect/local', function(req, res) {
            res.render('connect-local.ejs', { message: req.flash('loginMessage') });
        });
        app.post('/connect/local', passport.authenticate('local-signup', {
            successRedirect : '/profile', // redirect to the secure profile section
            failureRedirect : '/connect/local', // redirect back to the signup page if there is an error
            failureFlash : true // allow flash messages
        }));

    

// =============================================================================
// UNLINK ACCOUNTS =============================================================
// =============================================================================
// used to unlink accounts. for social accounts, just remove the token
// for local account, remove email and password
// user account will stay active in case they want to reconnect in the future

    // local -----------------------------------
    app.get('/unlink/local', isLoggedIn, function(req, res) {
        var user            = req.user;
        user.local.email    = undefined;
        user.local.password = undefined;
        user.save(function(err) {
            res.redirect('/profile');
        });
    });
    
};



// route middleware to ensure user is logged in
function isLoggedIn(req, res, next) {
    if (req.isAuthenticated())
        return next();

    res.redirect('/');
}
